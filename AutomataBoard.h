#pragma once
#include "DeviceResources.h"

using namespace  Microsoft::WRL;
using namespace DirectX;
using namespace std;

struct ComputeRootConst
{
	float kernel_width;
	float kernel_height;

	float board_width;
	float board_height;

	float image_width;
	float image_height;

	int dispatchX;
	int dispatchY;
	int dispatchZ;
	int square_size;
	ComputeRootConst(float kernel_width, float kernel_height, float board_width,
		float board_height, float image_width, float image_height, 
		int dispatchX, int dispatchY, int dispatchZ, int square_size)
		:kernel_width(kernel_width), kernel_height(kernel_height), board_width(board_width),
		board_height(board_height), image_width(image_width), image_height(image_height),
		dispatchX(dispatchX),dispatchY(dispatchY), dispatchZ(dispatchZ), square_size(square_size)
	{}

};

struct Float4
{
	float x;
	float y;
	float z;
	float w;
};
__declspec(align(1)) struct S_B_ranges
{
	/*
	Due unconstrained number of possible ranges each range is interpreted as a set [a,b]
	when there is a single value the set has to be [a,a]
	limit of ranges is up to 32 for both survive and birth
	padd is due to padding rules for HLSL  for this reasong keep number of sets %4 == 0
	*/
	int survive_range_count;
	int birth_range_count;
	int s_padd[2];
	Float4 boundaries[32]; // left survive, right survive, left birth, right birth

};

#define BOARD_BUFFER_COUNT 2
class AutomataBoard : public DeviceResources
{
public:
	AutomataBoard(HWND hwnd, int width, int height, int kernel_width = 1, int kernel_height = 1,
		float* init_board_state = nullptr, float* init_kernel_state = nullptr,
		int dispatchX = 10, int dispatchY = 10, int dispatchZ = 1, int square_size = 4, S_B_ranges* conditions = nullptr);
	void Resize(HWND hwnd);
	void Draw();
private:
	void CreateScreenTextureHeap();
	void SetUnorderedBuffers();
	void CreateBuffers();
	void BuildComputeRootSignature();
	void CreateComputePSO();
	void CreateScreenTexture();
private:
	int board_width, board_height;
	int kernel_width, kernel_height;
	int curr_board_buffer;
	float* kernel;
	S_B_ranges sb_range;
	ComputeRootConst meta_data;

	ComPtr<ID3D12DescriptorHeap> screen_texture_heap;

	ComPtr<ID3D12Resource> survive_birth_ranges;
	ComPtr<ID3D12Resource> compute_root_const;
	ComPtr<ID3D12Resource> upload_buffer;
	ComPtr<ID3D12Resource> board_buffer[BOARD_BUFFER_COUNT];
	ComPtr<ID3D12Resource> upload_kernel_buffer;
	ComPtr<ID3D12Resource> kernel_buffer;
	ComPtr<ID3D12Resource> screen_texture;
	ComPtr<ID3D12Resource> accumulated;

	ComPtr<ID3D12PipelineState> pipeline_ops;
	ComPtr<ID3D12PipelineState> pipeline_to_image;

	ComPtr<ID3D12RootSignature> root_signature_ops;
	ComPtr<ID3D12RootSignature> root_signature_to_image;

	UINT* map_upload_kernel_buffer;
	UINT* map_upload_buffer;
	UINT* map_compute_root_const;
	UINT* map_survive_birth_ranges;
};

