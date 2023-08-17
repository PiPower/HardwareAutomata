#pragma once
#include "AutomataBoard.h"

AutomataBoard* create_game_of_life(HWND hwnd, int board_width, int board_height, int square_size = 4);
AutomataBoard* create_bosco_automata(HWND hwnd, int board_width, int board_height, int square_size = 4);
AutomataBoard* create_bugsmovie_automata(HWND hwnd, int board_width, int board_height, int square_size = 4);
AutomataBoard* create_majority_automata(HWND hwnd, int board_width, int board_height, int square_size = 4);
AutomataBoard* create_waffle_automata(HWND hwnd, int board_width, int board_height, int square_size = 4);