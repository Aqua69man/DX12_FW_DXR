#include "Framework/Application.h"
#include <assert.h>

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
	std::wstring exePath = GetExePath();
	assert(exePath.size() != 0);

	const wchar_t* windowTitle = L"Learning DirectX 12";

	Application game(hInstance, windowTitle, 3500, 1800, false);
	game.Run();

	return 0;
}