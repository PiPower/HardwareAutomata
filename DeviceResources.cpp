#include "DeviceResources.h"
#include <iostream>
#include <string>
#include <d3dcompiler.h>
#include <array>
#include <random>
#include <comdef.h>
#include "d3dx12.h"
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#define NOT_SUCCEEDED(hr) if(hr != S_OK) {not_succeded(hr, __LINE__, __FILE__ ) ;}
using namespace std;

void not_succeded(HRESULT hr, int line, const char* path)
{
    _com_error err(hr);
    wstring error_msg = L"INVALID OPERATION\n";
    error_msg.append(L"LINE: ");
    error_msg.append(to_wstring(line));
    error_msg.append(L"\n");
    error_msg.append(L"FILE: ");
    wchar_t error_code[200];
    size_t char_count;
    mbstowcs_s(&char_count, error_code, (200 / sizeof(WORD)) * sizeof(wchar_t), __FILE__, 200);
    error_msg.append(error_code);
    error_msg.append(L"\n");
    error_msg.append(L"ERROR DESCRIPTION:\n");
    error_msg.append(err.ErrorMessage());
    MessageBox(NULL, error_msg.c_str(), NULL, MB_OK);
    exit(0);
}

DeviceResources::DeviceResources(HWND hwnd,int frame_count)
    :
    frame_count(frame_count), current_frame(0), renderTargets(frame_count)
{
#if defined(_DEBUG)
    // Enable the D3D12 debug layer.
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
        }
    }
#endif
    CreateDXGIFactory1(IID_PPV_ARGS(&factory));

    ComPtr<IDXGIAdapter> gpuAdapter;
    if (SUCCEEDED(factory->EnumAdapters(0, &gpuAdapter)))
    {
        DXGI_ADAPTER_DESC adapterDesc;
        gpuAdapter->GetDesc(&adapterDesc);
        OutputDebugString(L"CHOSEN ADAPTER IS: ");
        OutputDebugString(adapterDesc.Description);
        OutputDebugString(L"\n ");
    }
    else
    {
        OutputDebugString(L"INAVLID CALL TO EnumAdapters \n");
        exit(0);
    }

    NOT_SUCCEEDED(D3D12CreateDevice(gpuAdapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&Device)))

    NOT_SUCCEEDED(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));
    FenceValue = 1;
    // Create an event handle to use for frame synchronization.
    FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (FenceEvent == nullptr)
    {
        NOT_SUCCEEDED(HRESULT_FROM_WIN32(GetLastError()));
    }

    rtv_heap_size = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    cbv_srv_uav_heap_size = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    dsv_heap_size = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    sampler_heap_size = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    CreateCommandAllocatorListQueue();
    CreateDescriptorHeaps();
    CreateSwapChain(hwnd);
    Resize(hwnd);
}

void DeviceResources::Synchronize()
{
    // Signal and increment the fence value.
    const UINT64 fence = FenceValue;
    NOT_SUCCEEDED(CommandQueue->Signal(Fence.Get(), fence));
    FenceValue++;

    // Wait until the previous frame is finished.
    if (Fence->GetCompletedValue() < fence)
    {
        NOT_SUCCEEDED(Fence->SetEventOnCompletion(fence, FenceEvent));
        WaitForSingleObject(FenceEvent, INFINITE);
    }
}

void DeviceResources::CreateCommandAllocatorListQueue()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    NOT_SUCCEEDED(Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&CommandQueue)));

    NOT_SUCCEEDED(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CommandAllocator)));

    NOT_SUCCEEDED(Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CommandAllocator.Get(),
        nullptr, IID_PPV_ARGS(&CommandList)));
    CommandList->Close();
}

void DeviceResources::CreateDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = frame_count;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    NOT_SUCCEEDED(Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;
    NOT_SUCCEEDED(Device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&depthHeap)));
}

void DeviceResources::Resize(HWND window, void* object)
{
    DeviceResources* deviceResources = (DeviceResources*)object;
    deviceResources->Resize(window);
}

D3D12_CPU_DESCRIPTOR_HANDLE DeviceResources::CurrentBackBufferView()
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    current_frame = SwapChain->GetCurrentBackBufferIndex();
    handle.ptr += rtv_heap_size * current_frame;
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DeviceResources::CurrentDepthBufferView()
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = depthHeap->GetCPUDescriptorHandleForHeapStart();
    return handle;
}

void DeviceResources::Resize(HWND hwnd)
{
    Synchronize();

    RECT rc;
    GetClientRect(hwnd, &rc);
    this->width = rc.right - rc.left;
    this->height = rc.bottom - rc.top;

    this->ViewPort = { 0, 0,(float)this->width, (float)this->height,0 , 1.0 };

    this->ScissorRect = { 0, 0,  this->width, this->height };

    for (UINT n = 0; n < frame_count; n++)
    {
        renderTargets[n].Reset();
    }


    NOT_SUCCEEDED(SwapChain->ResizeBuffers(frame_count, this->width, this->height, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT n = 0; n < frame_count; n++)
    {
        NOT_SUCCEEDED(SwapChain->GetBuffer(n, IID_PPV_ARGS(&renderTargets[n])));
        Device->CreateRenderTargetView(renderTargets[n].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += rtv_heap_size;
    }

    D3D12_RESOURCE_DESC depthStencilDesc;
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = this->width;
    depthStencilDesc.Height = this->height;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;

    D3D12_HEAP_PROPERTIES heap_prop_depth = {};
    heap_prop_depth.Type = D3D12_HEAP_TYPE_DEFAULT;
    heap_prop_depth.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap_prop_depth.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heap_prop_depth.CreationNodeMask = 1;
    heap_prop_depth.VisibleNodeMask = 1;

    NOT_SUCCEEDED(Device->CreateCommittedResource(
        &heap_prop_depth,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(&DepthStencilBuffer)));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.Texture2D.MipSlice = 0;
    Device->CreateDepthStencilView(DepthStencilBuffer.Get(), &dsvDesc, depthHeap->GetCPUDescriptorHandleForHeapStart());

    CommandList->Reset(CommandAllocator.Get(), nullptr);
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        DepthStencilBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_DEPTH_WRITE);
    CommandList->ResourceBarrier(1, &barrier);

    NOT_SUCCEEDED(CommandList->Close());
    ID3D12CommandList* ppCommandLists[] = { CommandList.Get() };
    CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    this->Synchronize();

    this->current_frame = SwapChain->GetCurrentBackBufferIndex();
}

void DeviceResources::CreateSwapChain(HWND hwnd)
{

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = frame_count;
    swapChainDesc.BufferDesc.Width = this->width;
    swapChainDesc.BufferDesc.Height = this->height;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.OutputWindow = hwnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = TRUE;

    ComPtr<IDXGISwapChain> swapChain_buffer;
    factory->CreateSwapChain(CommandQueue.Get(), &swapChainDesc, &swapChain_buffer);

    NOT_SUCCEEDED(swapChain_buffer.As(&SwapChain));

    current_frame = SwapChain->GetCurrentBackBufferIndex();
}
