#include "Clear.h"
#include "..\DX12FrameWork\Utils\Utils.h"


int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
	std::wstring exePath = GetExeDirW();
	assert(exePath.size() != 0);

	const wchar_t* windowTitle = L"Learning DirectX 12";

	Clear game(hInstance, windowTitle, 3500, 1800, false);
	game.LoadContent();
	game.Run();
	game.UnloadContent();

	return 0;
}