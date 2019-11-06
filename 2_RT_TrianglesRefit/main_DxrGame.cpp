#include "DxrGame.h"

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
	std::wstring exePath = GetExeDirW();
	assert(exePath.size() != 0);

	const wchar_t* windowTitle = L"Learning DirectX 12";

	DxrGame game (hInstance, windowTitle, 3500, 1800, false);
	game.InitDXR();
	game.Run();

	return 0;
}