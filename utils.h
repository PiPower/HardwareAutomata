#pragma once
#include <D3d12.h>


void CopyDataTo(ID3D12Resource* dest, ID3D12Resource* src, D3D12_RESOURCE_STATES init_state_dst,
	D3D12_RESOURCE_STATES init_state_src, ID3D12GraphicsCommandList* cmd_list);
int AlignToConstBuffer(int size_of_struct, int align_value = 256);
void create_padded_board(float** buffer, int* width, int* height, int kernel_width, int kernel_height);
float* create_kernel(int width, int height);