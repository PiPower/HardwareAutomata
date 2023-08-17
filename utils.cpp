#include "utils.h"
#include "d3dx12.h"
#include <random>

void CopyDataTo(ID3D12Resource* dest, ID3D12Resource* src, D3D12_RESOURCE_STATES init_state_dst,
	D3D12_RESOURCE_STATES init_state_src, ID3D12GraphicsCommandList* cmd_list)
{
	D3D12_RESOURCE_BARRIER barrier_copy[2];
	barrier_copy[0] = CD3DX12_RESOURCE_BARRIER::Transition(dest, init_state_dst, D3D12_RESOURCE_STATE_COPY_DEST);
	barrier_copy[1] = CD3DX12_RESOURCE_BARRIER::Transition(src, init_state_src, D3D12_RESOURCE_STATE_COPY_SOURCE);

	D3D12_RESOURCE_BARRIER barrier_return[2];
	barrier_return[0] = CD3DX12_RESOURCE_BARRIER::Transition(dest, D3D12_RESOURCE_STATE_COPY_DEST, init_state_dst);
	barrier_return[1] = CD3DX12_RESOURCE_BARRIER::Transition(src, D3D12_RESOURCE_STATE_COPY_SOURCE, init_state_src);

	cmd_list->ResourceBarrier(2, barrier_copy);
	cmd_list->CopyResource(dest, src);
	cmd_list->ResourceBarrier(2, barrier_return);
}

int AlignToConstBuffer(int size_of_struct, int align_value)
{
	float ceiling = ceil((float)size_of_struct / align_value);
	return (int)ceiling * align_value;
}


void create_padded_board(float** buffer, int* width, int* height, int kernel_width, int kernel_height)
{
	int half_width = (kernel_width - 1) / 2;
	int half_height = (kernel_height - 1) / 2;

	*buffer = new float[(*width + half_width * 2) * (*height + half_height * 2)];
	memset(*buffer, 0, (*width + half_width * 2) * (*height + half_height * 2) * sizeof(float));
	std::random_device rd;
	std::mt19937_64 gen(rd());
	std::uniform_int_distribution<> distrib(0, 1);


	for (int i = 0; i < (*width) * (*height); i++)
	{
		float y = floor(i / (*width));
		float x = i - y * (*width);

		float buff_y = y + half_height;
		float buff_x = x + half_width;

		int buff_base_index = buff_x + (buff_y * (*width + half_width * 2));
		int random = distrib(gen);
		(*buffer)[buff_base_index] = random;
	}

	*width += half_width * 2;
	*height += half_height * 2;
}

float* create_kernel(int width, int height)
{
	float* kernel = new float[width * height];
	for (int i = 0; i < width * height; i++)
	{
		kernel[i] = 1.0f;
	}

	return kernel;
}
