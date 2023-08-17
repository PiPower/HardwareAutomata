#include "BoardCreators.h"
#include <random>
#include <string.h>
#include "utils.h"

AutomataBoard* create_game_of_life(HWND hwnd, int board_width, int board_height, int square_size)
{
	int kernel_width = 3, kernel_height = 3;
	float* kernel = create_kernel(kernel_width, kernel_height);
	float* board;
	create_padded_board(&board, &board_width, &board_height, kernel_width, kernel_height);
	AutomataBoard* lenia_game = new AutomataBoard(hwnd, board_width, board_height, kernel_width, kernel_height, board, kernel,
	10, 10, 1 , square_size);

	return lenia_game;
}

AutomataBoard* create_bosco_automata(HWND hwnd, int board_width, int board_height, int square_size)
{
	int kernel_width = 11, kernel_height = 11;
	float* kernel = create_kernel(kernel_width, kernel_height);
	float* board;
	create_padded_board(&board, &board_width, &board_height, kernel_width, kernel_height);
	S_B_ranges rules;
	rules.birth_range_count = 1;
	rules.survive_range_count = 1;
	rules.boundaries[0].x = 33;
	rules.boundaries[0].y = 57;
	rules.boundaries[0].z = 34;
	rules.boundaries[0].w = 45;

	AutomataBoard* lenia_game = new AutomataBoard(hwnd, board_width, board_height, kernel_width, kernel_height, board, kernel,
		10, 10, 1, square_size,&rules);

	return lenia_game;
}

AutomataBoard* create_bugsmovie_automata(HWND hwnd, int board_width, int board_height, int square_size)
{
	int kernel_width = 21, kernel_height = 21;
	float* kernel = create_kernel(kernel_width, kernel_height);
	float* board;
	create_padded_board(&board, &board_width, &board_height, kernel_width, kernel_height);
	S_B_ranges rules;
	rules.birth_range_count = 1;
	rules.survive_range_count = 1;
	rules.boundaries[0].x = 122;
	rules.boundaries[0].y = 221;
	rules.boundaries[0].z = 123;
	rules.boundaries[0].w = 170;

	AutomataBoard* lenia_game = new AutomataBoard(hwnd, board_width, board_height, kernel_width, kernel_height, board, kernel,
		10, 10, 1, square_size, &rules);

	return lenia_game;
}

AutomataBoard* create_majority_automata(HWND hwnd, int board_width, int board_height, int square_size)
{
	int kernel_width = 9, kernel_height = 9;
	float* kernel = create_kernel(kernel_width, kernel_height);
	float* board;
	create_padded_board(&board, &board_width, &board_height, kernel_width, kernel_height);
	S_B_ranges rules;
	rules.survive_range_count = 1;
	rules.birth_range_count = 1;
	rules.boundaries[0].x = 40;
	rules.boundaries[0].y = 80;
	rules.boundaries[0].z = 41;
	rules.boundaries[0].w = 81;

	AutomataBoard* lenia_game = new AutomataBoard(hwnd, board_width, board_height, kernel_width, kernel_height, board, kernel,
		10, 10, 1, square_size, &rules);

	return lenia_game;
}

AutomataBoard* create_waffle_automata(HWND hwnd, int board_width, int board_height, int square_size)
{
	int kernel_width = 15, kernel_height = 15;
	float* kernel = create_kernel(kernel_width, kernel_height);
	float* board;
	create_padded_board(&board, &board_width, &board_height, kernel_width, kernel_height);
	S_B_ranges rules;
	rules.survive_range_count = 1;
	rules.birth_range_count = 1;
	rules.boundaries[0].x = 99;
	rules.boundaries[0].y = 199;
	rules.boundaries[0].z = 75;
	rules.boundaries[0].w = 170;

	AutomataBoard* lenia_game = new AutomataBoard(hwnd, board_width, board_height, kernel_width, kernel_height, board, kernel,
		10, 10, 1, square_size, &rules);

	return lenia_game;
}
