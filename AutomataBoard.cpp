#include "AutomataBoard.h"
#include "d3dx12.h"
#include <string.h>
#include "utils.h"
#include <d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")

AutomataBoard::AutomataBoard(HWND hwnd, int width, int height, int kernel_width, int kernel_height,
	float* init_board_state, float* init_kernel_state, int dispatchX, int dispatchY, int dispatchZ, int square_size, S_B_ranges* conditions)
	:
	DeviceResources(hwnd), board_width(width), board_height(height), kernel_width(kernel_width), 
	kernel_height(kernel_height), curr_board_buffer(0),
	meta_data( kernel_width, kernel_height, board_width, height, this->width, this->height, dispatchX, dispatchY, dispatchZ, square_size)
{
	kernel = new float[kernel_width * kernel_height];
	CreateBuffers();
	BuildComputeRootSignature();
	CreateScreenTexture();
	CreateScreenTextureHeap();
	CreateComputePSO();

	UINT8* pVertexDataBegin;
	D3D12_RANGE read_range = { 0,0 };
	NOT_SUCCEEDED(upload_buffer->Map(0, &read_range, (void**)&map_upload_buffer));
	NOT_SUCCEEDED(upload_kernel_buffer->Map(0, &read_range, (void**)&map_upload_kernel_buffer));
	NOT_SUCCEEDED(compute_root_const->Map(0, &read_range, (void**)&map_compute_root_const));
	NOT_SUCCEEDED(survive_birth_ranges->Map(0, &read_range, (void**)&map_survive_birth_ranges));

	Synchronize();
	NOT_SUCCEEDED( CommandAllocator->Reset() );
	NOT_SUCCEEDED( CommandList->Reset(CommandAllocator.Get(), nullptr) );
	if (init_board_state != nullptr)
	{
		memcpy(map_upload_buffer, init_board_state, sizeof(float) * board_width * board_height);
		CopyDataTo(board_buffer[curr_board_buffer].Get(), upload_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, 
			D3D12_RESOURCE_STATE_GENERIC_READ, CommandList.Get());
	}

	if (init_kernel_state != nullptr)
	{
		memcpy(kernel, init_kernel_state, sizeof(float) * kernel_width * kernel_height);
		memcpy(map_upload_kernel_buffer, kernel, sizeof(float) * kernel_width * kernel_height);
		CopyDataTo(kernel_buffer.Get(), upload_kernel_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, 
			D3D12_RESOURCE_STATE_GENERIC_READ, CommandList.Get());

	}

	memset(&sb_range, 0, sizeof(S_B_ranges));
	if (conditions == nullptr)
	{
		sb_range.survive_range_count = 1;
		sb_range.birth_range_count = 1;

		sb_range.boundaries[0].x = 2.0;
		sb_range.boundaries[0].y = 3.0;

		sb_range.boundaries[0].z = 3.0;
		sb_range.boundaries[0].w = 3.0;

	}
	else memcpy(&sb_range, conditions, sizeof(S_B_ranges));

	memcpy(map_compute_root_const, &meta_data, sizeof(ComputeRootConst));
	memcpy(map_survive_birth_ranges, &sb_range, sizeof(S_B_ranges));

	SetUnorderedBuffers();
	CommandList->Close();
	ID3D12CommandList* executable[] = {CommandList.Get()};
	CommandQueue->ExecuteCommandLists(1, executable);
	Synchronize();
}

void AutomataBoard::Resize(HWND hwnd)
{
	this->DeviceResources::Resize(hwnd);

	CreateScreenTexture();
	CreateScreenTextureHeap();

	Synchronize();
	NOT_SUCCEEDED(CommandAllocator->Reset());
	NOT_SUCCEEDED(CommandList->Reset(CommandAllocator.Get(), nullptr));

	D3D12_RESOURCE_BARRIER barrier_UAV[1];
	barrier_UAV[0] = CD3DX12_RESOURCE_BARRIER::Transition(screen_texture.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	CommandList->ResourceBarrier(1, barrier_UAV);

	CommandList->Close();
	ID3D12CommandList* executable[] = { CommandList.Get() };
	CommandQueue->ExecuteCommandLists(1, executable);
	Synchronize();

	meta_data.image_height = height;
	meta_data.image_width = width;
	memcpy(map_compute_root_const, &meta_data, sizeof(ComputeRootConst));
}

void AutomataBoard::Draw()
{
	D3D12_RESOURCE_BARRIER barriers[2];
	barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barriers[0].UAV.pResource = board_buffer[curr_board_buffer].Get();

	barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barriers[1].UAV.pResource = accumulated.Get();



	NOT_SUCCEEDED(CommandAllocator->Reset());
	NOT_SUCCEEDED(CommandList->Reset(CommandAllocator.Get(), pipeline_ops.Get()));

	ID3D12DescriptorHeap* descriptorHeaps[] = { screen_texture_heap.Get() };
	CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	// Board update step
	CommandList->SetComputeRootSignature(root_signature_ops.Get());
	CommandList->SetComputeRootUnorderedAccessView(0, kernel_buffer->GetGPUVirtualAddress());
	CommandList->SetComputeRootUnorderedAccessView(1, board_buffer[curr_board_buffer]->GetGPUVirtualAddress());
	CommandList->SetComputeRootUnorderedAccessView(2, board_buffer[(curr_board_buffer + 1)% BOARD_BUFFER_COUNT]->GetGPUVirtualAddress());
	CommandList->SetComputeRootConstantBufferView(3, compute_root_const->GetGPUVirtualAddress());
	CommandList->SetComputeRootConstantBufferView(4, survive_birth_ranges->GetGPUVirtualAddress());
	CommandList->SetComputeRootUnorderedAccessView(5, accumulated->GetGPUVirtualAddress());

	CommandList->Dispatch(meta_data.dispatchX, meta_data.dispatchY, meta_data.dispatchZ);
	//create image from calculated buffer
	CommandList->SetPipelineState(pipeline_to_image.Get());
	CommandList->SetComputeRootSignature(root_signature_to_image.Get());
	CommandList->SetComputeRootUnorderedAccessView(0, board_buffer[(curr_board_buffer + 1) % BOARD_BUFFER_COUNT]->GetGPUVirtualAddress());
	CommandList->SetComputeRootDescriptorTable(1, screen_texture_heap->GetGPUDescriptorHandleForHeapStart());
	CommandList->SetComputeRootConstantBufferView(2, compute_root_const->GetGPUVirtualAddress());
	CommandList->SetComputeRootUnorderedAccessView(3, accumulated->GetGPUVirtualAddress());
	CommandList->ResourceBarrier(2, barriers);

	CommandList->Dispatch(meta_data.dispatchX, meta_data.dispatchY, meta_data.dispatchZ);
	CopyDataTo(renderTargets[current_frame].Get(), screen_texture.Get(), 
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, CommandList.Get());

	CommandList->Close();
	ID3D12CommandList* executable[] = { CommandList.Get() };
	CommandQueue->ExecuteCommandLists(1, executable);

	NOT_SUCCEEDED(SwapChain->Present(0, 0));
	Synchronize();
	current_frame = SwapChain->GetCurrentBackBufferIndex();
	curr_board_buffer = (curr_board_buffer + 1) % BOARD_BUFFER_COUNT;
}


void AutomataBoard::CreateScreenTextureHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	heap_desc.NumDescriptors = 1;
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	NOT_SUCCEEDED(Device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&screen_texture_heap)));

	
	DXGI_SWAP_CHAIN_DESC backBuffDesc;
	SwapChain->GetDesc(&backBuffDesc);
	D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
	uav_desc.Format = backBuffDesc.BufferDesc.Format;
	uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uav_desc.Texture2D.MipSlice = 0;
	Device->CreateUnorderedAccessView(screen_texture.Get(), nullptr, &uav_desc, screen_texture_heap->GetCPUDescriptorHandleForHeapStart());
}

void AutomataBoard::SetUnorderedBuffers()
{
	D3D12_RESOURCE_BARRIER barrier_UAV[BOARD_BUFFER_COUNT+3];
	for (int i = 0; i < BOARD_BUFFER_COUNT; i++)
	{
		barrier_UAV[i] = CD3DX12_RESOURCE_BARRIER::Transition(board_buffer[i].Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}
	barrier_UAV[BOARD_BUFFER_COUNT] = CD3DX12_RESOURCE_BARRIER::Transition(kernel_buffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	barrier_UAV[BOARD_BUFFER_COUNT+1 ] = CD3DX12_RESOURCE_BARRIER::Transition(screen_texture.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	barrier_UAV[BOARD_BUFFER_COUNT + 2] = CD3DX12_RESOURCE_BARRIER::Transition(accumulated.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	CommandList->ResourceBarrier(BOARD_BUFFER_COUNT + 3, barrier_UAV);
}

void AutomataBoard::CreateBuffers()
{
	for (int i = 0; i < BOARD_BUFFER_COUNT; i++)
	{
		CreateDefaultBuffer(&board_buffer[i], board_width * board_height * sizeof(float), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	}

	CreateDefaultBuffer(&accumulated, board_width * board_height * sizeof(float), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	CreateDefaultBuffer(&kernel_buffer, kernel_width * kernel_height * sizeof(float), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	CreateUploadBuffer(&upload_buffer, board_width * board_height * sizeof(float));
	CreateUploadBuffer(&upload_kernel_buffer, kernel_width * kernel_height * sizeof(float));
	CreateUploadBuffer(&compute_root_const, AlignToConstBuffer(sizeof(ComputeRootConst)));
	CreateUploadBuffer(&survive_birth_ranges, AlignToConstBuffer(sizeof(S_B_ranges)));
}

void AutomataBoard::BuildComputeRootSignature()
{

	CD3DX12_ROOT_PARAMETER1 rootParametersOps[6];
	rootParametersOps[0].InitAsUnorderedAccessView(0);
	rootParametersOps[1].InitAsUnorderedAccessView(1);
	rootParametersOps[2].InitAsUnorderedAccessView(2);
	rootParametersOps[3].InitAsConstantBufferView(0);
	rootParametersOps[4].InitAsConstantBufferView(1);
	rootParametersOps[5].InitAsUnorderedAccessView(3);

	CD3DX12_DESCRIPTOR_RANGE1 uavTable;
	uavTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);

	CD3DX12_ROOT_PARAMETER1 rootParametersToImage[4];
	rootParametersToImage[0].InitAsUnorderedAccessView(0);
	rootParametersToImage[1].InitAsDescriptorTable(1, &uavTable);
	rootParametersToImage[2].InitAsConstantBufferView(0);
	rootParametersToImage[3].InitAsUnorderedAccessView(3);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureOpsDesc, rootSignatureToImageDesc;
	rootSignatureOpsDesc.Init_1_1(_countof(rootParametersOps), rootParametersOps, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);
	rootSignatureToImageDesc.Init_1_1(_countof(rootParametersToImage), rootParametersToImage, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;

	NOT_SUCCEEDED(D3DX12SerializeVersionedRootSignature(&rootSignatureOpsDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &signature, &error));
	if (error.Get() != nullptr)
	{
		MessageBox(NULL, L"ROOT SIGNATURE ERROR", NULL, MB_OK);
		exit(-1);
	}
	NOT_SUCCEEDED(Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&root_signature_ops)));

	NOT_SUCCEEDED(D3DX12SerializeVersionedRootSignature(&rootSignatureToImageDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &signature, &error));
	if (error.Get() != nullptr)
	{
		MessageBox(NULL, L"ROOT SIGNATURE ERROR", NULL, MB_OK);
		exit(-1);
	}
	NOT_SUCCEEDED(Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&root_signature_to_image)));
}

void AutomataBoard::CreateComputePSO()
{
	ComPtr<ID3DBlob> shader_byte_code;
	ComPtr<ID3DBlob> error;
#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	D3DCompileFromFile(L"Shaders\\ComputeShaders.hlsl", NULL, NULL, "board_update", "cs_5_0", compileFlags, 0, &shader_byte_code, &error);
	if (error.Get() != nullptr )
	{
		wchar_t error_code[400];
		size_t char_count;
		mbstowcs_s(&char_count, error_code, (400 / sizeof(WORD)) * sizeof(wchar_t), (char*)error->GetBufferPointer(), 400);
		if (shader_byte_code.Get() == nullptr)
		{
			MessageBox(NULL, error_code, NULL, MB_OK);
			exit(-1);
		}
		else
		{
			OutputDebugString(error_code);
		}
	}

	D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};
	computePsoDesc.pRootSignature = root_signature_ops.Get();
	computePsoDesc.CS =
	{
		reinterpret_cast<BYTE*>(shader_byte_code->GetBufferPointer()),shader_byte_code->GetBufferSize()
	};

	computePsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	NOT_SUCCEEDED(Device->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&pipeline_ops)));

	D3DCompileFromFile(L"Shaders\\ComputeShaders.hlsl", NULL, NULL, "to_image", "cs_5_0", compileFlags, 0, &shader_byte_code, &error);
	if (error.Get() != nullptr)
	{
		wchar_t error_code[400];
		size_t char_count;
		mbstowcs_s(&char_count, error_code, (400 / sizeof(WORD)) * sizeof(wchar_t), (char*)error->GetBufferPointer(), 400);
		if (shader_byte_code.Get() == nullptr)
		{
			MessageBox(NULL, error_code, NULL, MB_OK);
			exit(-1);
		}
		else
		{
			OutputDebugString(error_code);
		}
	}

	computePsoDesc.pRootSignature = root_signature_to_image.Get();
	computePsoDesc.CS =
	{
		reinterpret_cast<BYTE*>(shader_byte_code->GetBufferPointer()),shader_byte_code->GetBufferSize()
	};
	NOT_SUCCEEDED(Device->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&pipeline_to_image)));
}

void AutomataBoard::CreateScreenTexture()
{
;
	D3D12_RESOURCE_DESC buffDesc = renderTargets[0]->GetDesc();;
	buffDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	D3D12_HEAP_PROPERTIES heap_prop_buffers = {};
	heap_prop_buffers.Type = D3D12_HEAP_TYPE_DEFAULT;
	heap_prop_buffers.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heap_prop_buffers.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heap_prop_buffers.CreationNodeMask = 1;
	heap_prop_buffers.VisibleNodeMask = 1;

	NOT_SUCCEEDED(Device->CreateCommittedResource(&heap_prop_buffers, D3D12_HEAP_FLAG_NONE,
		&buffDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&screen_texture)))

}
