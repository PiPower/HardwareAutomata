RWStructuredBuffer<float> kernel : register(u0);
RWStructuredBuffer<float> source_board : register(u1);
RWStructuredBuffer<float> target_board : register(u2);
RWStructuredBuffer<float> accumulated : register(u3);

cbuffer image_sizes : register(b0)
{
    float kernel_width;
    float kernel_height;
    
    float buffer_width;
    float buffer_height;
    
    float image_width;
    float image_height;
    
    int dispatchX;
    int dispatchY;
    int dispatchZ;
    
    int square_size;
}

cbuffer ranges : register(b1)
{
    int survive_range_count;
    int birth_range_count;
    float4 boundaries[32];
}
    
[numthreads(1024, 1, 1)]
void board_update(int3 threadIdx : SV_GroupThreadID, int3 BlockIdx : SV_GroupID)
{
    int total_threads = 1024 * dispatchX * dispatchY * dispatchZ;
    int total_thread_idx = threadIdx.x + BlockIdx.x * 1024 + 1024 * dispatchX * BlockIdx.y +
    1024 * dispatchX * dispatchY * BlockIdx.z;
    
    float half_width = (kernel_width - 1.0) / 2.0;
    float half_height = (kernel_height - 1.0) / 2.0;
    
    int board_width = buffer_width - kernel_width + 1;
    int board_height = buffer_height - kernel_height + 1;
    
    while (total_thread_idx < board_width * board_height)
    {
        float y = floor(total_thread_idx / board_width);
        float x = total_thread_idx - y * board_width;
        
        int buffer_x = x + half_width;
        int buffer_y = y + half_height;
    
        float accumulate = 0;
        for (int kerr_y = 0; kerr_y < kernel_height; kerr_y++)
        {
            for (int kerr_x = 0; kerr_x < kernel_width; kerr_x++)
            {
                accumulate += kernel[kerr_x + kerr_y * kernel_width] * source_board[(x + kerr_x) + (y + kerr_y) * buffer_width];
            }
        }
        float current_cell_state = source_board[buffer_x + buffer_y * buffer_width];
        accumulate = accumulate - current_cell_state * kernel[half_width + half_height * kernel_width];

        float updated_value = 0;
        /*
        // survive    
        if (current_cell_state == 1 && (accumulate == 2 || accumulate ==3))
            updated_value = 1;
        //birth
        if (current_cell_state == 0 && accumulate == 3)
            updated_value = 1;
        */
        
        int i = 0;
        while (current_cell_state == 1 && i < survive_range_count)
        {
            if (boundaries[i].x <= accumulate && accumulate <= boundaries[i].y)
            {
                updated_value = 1;
                break;
            }
            i++;
        }
        
        while (current_cell_state == 0 && i < birth_range_count)
        {
            if (boundaries[i].z <= accumulate && accumulate <= boundaries[i].w)
            {
                updated_value = 1;
                break;
            }
            i++;
        }
        
        accumulated[x + y * board_width] = accumulate;
        target_board[buffer_x + buffer_y * buffer_width] = updated_value;
        total_thread_idx += total_threads;
    }
    
}

RWStructuredBuffer<float> cell_board : register(u0);
RWTexture2D<float4> target_texture : register(u1);


[numthreads(1024, 1, 1)]
void to_image(int3 threadIdx : SV_GroupThreadID, int3 BlockIdx : SV_GroupID)
{
    int total_threads = 1024 * dispatchX * dispatchY * dispatchZ;
    int total_thread_idx = threadIdx.x + BlockIdx.x * 1024 + 1024 * dispatchX * BlockIdx.y + 
    1024 * dispatchX * dispatchY * BlockIdx.z;
    
    float half_width = (kernel_width - 1.0) / 2.0;
    float half_height = (kernel_height - 1.0) / 2.0;
    
    int board_width = buffer_width - kernel_width + 1;
    int board_height = buffer_height - kernel_height + 1;
    
    while (total_thread_idx < board_width * board_height)
    {
        float y = floor(total_thread_idx / board_width);
        float x = total_thread_idx - y * board_width;
        
        int buffer_x = x + half_width;
        int buffer_y = y + half_height;
        
        int buffer_index = buffer_x + buffer_y * (buffer_width);
        float curr_board_value = cell_board[buffer_index];
        int texture_x = x * square_size;
        int texture_y = y * square_size;
        for (int delta_y = 0; delta_y < square_size; delta_y++)
        {
            for (int delta_x = 0; delta_x < square_size; delta_x++)
            {
                float accumulate = accumulated[x + y * board_width];
                float3 color = float3(sin(accumulate) * cos(accumulate), sin(accumulate) * (1 / log(accumulate)), sin(accumulate)) * curr_board_value;
                target_texture[int2(texture_x + delta_x, texture_y + delta_y)] = float4(color, 1.0f);
            }

        }
        total_thread_idx += total_threads;
    }
}