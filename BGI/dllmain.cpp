// dllmain.cpp : 定义 DLL 应用程序的入口点。
//For 11月のアルカディア
#include "stdafx.h"
#include <windows.h>
#include <fstream>
#include <iostream>
#include <string>
#include "detours.h"
#pragma comment(lib, "detours.lib")
using namespace std;
ofstream TXT("Text.txt");

typedef HFONT(WINAPI *fnCreateFontA)(
	int nHeight, // logical height of font height
	int nWidth, // logical average character width
	int nEscapement, // angle of escapement
	int nOrientation, // base-line orientation angle
	int fnWeight, // font weight
	DWORD fdwItalic, // italic attribute flag
	DWORD fdwUnderline, // underline attribute flag
	DWORD fdwStrikeOut, // strikeout attribute flag
	DWORD fdwCharSet, // character set identifier
	DWORD fdwOutputPrecision, // output precision
	DWORD fdwClipPrecision, // clipping precision
	DWORD fdwQuality, // output quality
	DWORD fdwPitchAndFamily, // pitch and family
	LPCSTR lpszFace // pointer to typeface name string
	);
fnCreateFontA CreateFontAOLD;
HFONT WINAPI CreateFontAEx(int nHeight, int nWidth, int nEscapement, int nOrientation, int fnWeight, DWORD fdwItalic, DWORD fdwUnderline, DWORD fdwStrikeOut, DWORD fdwCharSet, DWORD fdwOutputPrecision, DWORD fdwClipPrecision, DWORD fdwQuality, DWORD fdwPitchAndFamily, LPCSTR lpszFace)
{
	fdwCharSet = 0x86;
	fnWeight = FW_SEMIBOLD;
	return CreateFontAOLD(nHeight, nWidth, nEscapement, nOrientation, fnWeight, fdwItalic, fdwUnderline, fdwStrikeOut, fdwCharSet, fdwOutputPrecision, fdwClipPrecision, fdwQuality, fdwPitchAndFamily, "黑体");
}


LPWSTR ctowJIS(char* str)
{
	DWORD dwMinSize;
	dwMinSize = MultiByteToWideChar(932, 0, str, -1, NULL, 0); //计算长度
	LPWSTR out = new wchar_t[dwMinSize];
	MultiByteToWideChar(932, 0, str, -1, out, dwMinSize);//转换
	return out;
}

char* wtocGBK(LPCTSTR str)
{
	DWORD dwMinSize;
	dwMinSize = WideCharToMultiByte(936, NULL, str, -1, NULL, 0, NULL, FALSE); //计算长度
	char *out = new char[dwMinSize];
	WideCharToMultiByte(936, NULL, str, -1, out, dwMinSize, NULL, FALSE);//转换
	return out;
}

static void make_console() {
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
	freopen("CONIN$", "r", stdin);
	cout << "Console\n" << endl;
}

int (*OldStr)(int a1, DWORD *a2, int a3, BYTE *a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12, int a13);
int  MyStr(int a1, DWORD *a2, int a3, BYTE *a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12, int a13)
{
	_asm
	{
		pushad
	}
	char* ret;
	int index;
	int rep;
	ret = (char*)a2;
	index = *(int*)ret;
	char* Pstr = (char*)ret + sizeof(int)-4;
	cout <<  index << "|" << wtocGBK(ctowJIS(Pstr)) << endl;
	TXT <<  index << "|" << wtocGBK(ctowJIS(Pstr)) << endl;
	_asm
	{
		popad
	}
	return OldStr(a1, a2, a3, a4,a5,a6,a7,a8,a9,a10,a11,a12,a13);
}
void Hook()
{
	DWORD BaseAddr = (DWORD)GetModuleHandle(NULL);
	cout << "BaseAddress:0x" << hex<< BaseAddr << endl;
	TXT << "BaseAddress:0x" <<hex << BaseAddr << endl;
	*(DWORD*)&OldStr = BaseAddr + 0x32d20;
	cout << "HookAddress:0x" << hex << (BaseAddr + 0x32d20) <<"\n"<< endl;
	TXT << "HookAddress:0x" << hex << (BaseAddr + 0x32d20) << endl;
	CreateFontAOLD = (fnCreateFontA)GetProcAddress(GetModuleHandle(L"gdi32.dll"), "CreateFontA");
	DetourTransactionBegin();
	DetourAttach((void**)&OldStr, MyStr);
	DetourAttach((void**)&CreateFontAOLD, CreateFontAEx);
	if(DetourTransactionCommit()!=NOERROR){ MessageBox(NULL, L"ERROR", L"Crescendo", MB_OK); }
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		make_console();
		Hook();
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

extern "C" __declspec(dllexport) void dummy(void) {
	return;
}

