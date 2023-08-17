#include <iostream>
#include "DeviceResources.h"
#include "AutomataBoard.h"
#include "window.h"
#include <random>
#include "BoardCreators.h"
using namespace std;

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
	int screen_width = 1000, screen_height = 1000;
	Window window(screen_width, screen_height, L"Klasa", L"Lenia");
	AutomataBoard* leniaAutomata = create_game_of_life(window.GetWindowHWND(), 500, 500,2);
	window.RegisterResizezable(leniaAutomata, DeviceResources::Resize);
	while (window.ProcessMessages() == 0)
	{
		leniaAutomata->Draw();
		Sleep(16);
	}

	return 0;
}

