#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h> // For HRESULT
#include <exception> // For std::exception
#include <string>    // For msgBox
#include <codecvt>   // For wstring_convert

// From DXSampleHelper.h 
// Source: https://github.com/Microsoft/DirectX-Graphics-Samples
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