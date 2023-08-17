#pragma once

#include "window.h"
#include <dxgi1_4.h>
#include <d3d12.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <vector>

void not_succeded(HRESULT hr, int line, const char* path);
#define NOT_SUCCEEDED(hr) if(hr != S_OK) {not_succeded(hr, __LINE__, __FILE__ ) ;}

using namespace  Microsoft::WRL;
using namespace DirectX;
using namespace std;

class DeviceResources
{
public:
	DeviceResources(HWND hwnd, int frame_count = 3);
	void static Resize(HWND window, void* object);
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView();
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentDepthBufferView();
	void CreateDefaultBuffer(ID3D12Resource** Resource, int size, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
	void CreateUploadBuffer(ID3D12Resource** Resource, int size, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
	ID3D12Resource* CreateDefaultBuffer(int size, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
	ID3D12Resource* CreateUploadBuffer(int size, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
	void Synchronize();
	virtual ~DeviceResources(){}
protected:
	void CreateCommandAllocatorListQueue();
	void CreateDescriptorHeaps();
	virtual void Resize(HWND hwnd);
	void CreateSwapChain(HWND hwnd);
protected:
	ComPtr<IDXGIFactory4> factory;
	ComPtr<IDXGISwapChain3> SwapChain;
	ComPtr<ID3D12Device> Device;
	ComPtr<ID3D12DescriptorHeap> rtvHeap;
	ComPtr<ID3D12DescriptorHeap> depthHeap;
	vector<ComPtr<ID3D12Resource> > renderTargets;
	ComPtr<ID3D12CommandQueue> CommandQueue;
	ComPtr<ID3D12CommandAllocator> CommandAllocator;
	ComPtr<ID3D12GraphicsCommandList> CommandList;
	ComPtr<ID3D12Resource> DepthStencilBuffer;

	D3D12_VIEWPORT ViewPort;
	D3D12_RECT ScissorRect;

	HANDLE FenceEvent;
	ComPtr<ID3D12Fence> Fence;
	UINT64 FenceValue;

	int width, height;
	int frame_count, current_frame;
	int rtv_heap_size, cbv_srv_uav_heap_size;
	int dsv_heap_size,sampler_heap_size;
};


