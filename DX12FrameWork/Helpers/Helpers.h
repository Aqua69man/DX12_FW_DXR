#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h> // For HRESULT
#include <exception> // For std::exception
#include <string>    // For msgBox
#include <codecvt>   // For wstring_convert

inline std::wstring GetExePath()
{
	WCHAR path[MAX_PATH];
	HMODULE hModule = GetModuleHandleW(NULL);
	if (GetModuleFileNameW(hModule, path, MAX_PATH) > 0)
	{
		std::wstring wstr = std::wstring(path);
		size_t pos = wstr.rfind('\\');
		if (pos != std::string::npos) {
			auto str = wstr.substr(0, pos + 1);
			return str;
		}
	}
	return std::wstring(L"");
}

inline std::string GetWorkingDirPath()
{
	CHAR path[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, path);
	std::string wPath = std::string(path);
	return wPath;
}

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception();
	}
}

inline std::wstring string_2_wstring(const std::string& s)
{
	std::wstring_convert<std::codecvt_utf8<WCHAR>> cvt;
	std::wstring ws = cvt.from_bytes(s);
	return ws;
}

inline std::string wstring_2_string(const std::wstring& ws)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> cvt;
	std::string s = cvt.to_bytes(ws);
	return s;
}

inline void MsgBox(const std::string& msg)
{
	HWND gWinHandle = nullptr;
	MessageBoxA(gWinHandle, msg.c_str(), "Error", MB_OK);
}