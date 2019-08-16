// dllmain.cpp : 定义 DLL 应用程序的入口点。
//mb2wc_Hook在krkr2上的实例，实际效果请参考《向日葵 Aqua After》的汉化。
//部分注释可以在测试时打开，实际上这个可以当做krkr2的debug mode。
#include "stdafx.h"
#include "detours.h"
#include <fstream>
#include <iostream>
#include <map>
#include <vector>
#include <sstream>
#include "crc32.h"
#include <iomanip>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#pragma comment(lib, "detours.lib")
using namespace std;

//ofstream flog("SCR.txt");

//map<DWORD, wstring> SCRList;
map<DWORD, wstring> REPList;

PVOID g_pOldMultiByteToWideChar = NULL;
typedef int(WINAPI *PfuncMultiByteToWideChar)(
	_In_      UINT   CodePage,
	_In_      DWORD  dwFlags,
	_In_      LPCSTR lpMultiByteStr,
	_In_      int    cbMultiByte,
	_Out_opt_ LPWSTR lpWideCharStr,
	_In_      int    cchWideChar);

char* wtocUTF(LPCTSTR str)
{
	DWORD dwMinSize;
	dwMinSize = WideCharToMultiByte(CP_UTF8, NULL, str, -1, NULL, 0, NULL, FALSE); //计算长度
	char *out = new char[dwMinSize];
	WideCharToMultiByte(CP_UTF8, NULL, str, -1, out, dwMinSize, NULL, FALSE);//转换
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

LPWSTR ctowUTF(char* str)
{
	DWORD dwMinSize;
	dwMinSize = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0); //计算长度
	LPWSTR out = new wchar_t[dwMinSize];
	MultiByteToWideChar(CP_UTF8, 0, str, -1, out, dwMinSize);//转换
	return out;
}


static void make_console() {
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
	freopen("CONIN$", "r", stdin);
	cout << "Crescendo Ver 1.0.4\n\n" << endl;
}
int WINAPI NewMultiByteToWideChar(UINT cp, DWORD dwFg, LPCSTR lpMBS, int cbMB, LPWSTR lpWCS, int ccWC)
{
	__asm
	{
		pushad
	}
	CRC32 crc;
	int ret = 0;
	ret = ((PfuncMultiByteToWideChar)g_pOldMultiByteToWideChar)(cp, dwFg, lpMBS, cbMB, lpWCS, ccWC);
	if (lpWCS != 0 && (USHORT)*lpWCS > 0x20)//检测所有的有效字符
	{
			wstring wstr = lpWCS;
			DWORD strcrc = crc.Calc((char*)lpWCS, ret);
			auto scitr = REPList.find(strcrc);
			if (scitr!= REPList.end())
			{
				wcscpy(lpWCS, scitr->second.c_str());
				ret = scitr->second.length();
				//cout << "REPLACE:" << "0x" << hex << scitr->first << wtocGBK(scitr->second.c_str()) << endl;
			}
			/*else
			{
				auto itr = SCRList.find(strcrc);
				if (itr == SCRList.end())
				{
					flog << "0x" << hex << setw(8) << setfill('0') << strcrc << "|" << wtocGBK(wstr.c_str()) << endl;
					cout << "0x" << hex << setw(8) << setfill('0') << strcrc << "|" << wtocGBK(wstr.c_str()) << endl;
					SCRList.insert(pair<DWORD, wstring>(strcrc, wstr));
					
				}
			}*/
		}
	__asm
	{
		popad
	}
	return ret;
}

void LoadStringMap()
{
	ifstream fin("Crescendo.ini");
	const int LineMax = 0x99999;//其实用不到这么大，懒得计算了而已
	char str[LineMax];
	if (fin.is_open())
	{
		while (fin.getline(str,LineMax))
		{
			auto wtmp = ctowUTF(str);
			wstring wline = wtmp;
			wstring crcval = wline.substr(2, 8);
			wstring wstr = wline.substr(11);
			DWORD crc = wcstoul(crcval.c_str(), NULL, 16);
			//cout << "LOAD:" <<"0x"<<hex<<crc<< wtocGBK(wstr.c_str()) << endl;
			REPList.insert(pair<DWORD, wstring>(crc, wstr));
		}
		//MessageBox(0, L"配置文件读取成功", L"CrescendoSystem", MB_OK);
	}
	else
	{
		MessageBox(0, L"配置文件读取失败", L"ERROR", MB_OK);
	}
}

void HookStart() 
{
	g_pOldMultiByteToWideChar = DetourFindFunction("Kernel32.dll", "MultiByteToWideChar");
	DetourTransactionBegin();
	DetourAttach(&g_pOldMultiByteToWideChar, NewMultiByteToWideChar);
	DetourTransactionCommit();
}

void Photo()
{
	char fileName[]="staff.dzi";                        //定义打开图像名字
	char *buf;                                //定义文件读取缓冲区
	char *p;
	int r, g, b, pix;
	HWND wnd;                                 //窗口句柄
	HDC dc;                                   //绘图设备环境句柄
	FILE *fp;                                 //定义文件指针
	FILE *fpw;                                //定义保存文件指针
	DWORD w, h;                                //定义读取图像的长和宽
	DWORD bitCorlorUsed;                      //定义
	DWORD bitSize;                            //定义图像的大小
	BITMAPFILEHEADER bf;                      //图像文件头
	BITMAPINFOHEADER bi;                      //图像文件头信息
	if ((fp = fopen(fileName, "rb")) == NULL)
	{
		MessageBox(0, L"缺少staff文件", L"ERROR", MB_OK);
		exit(0);
	}
	fread(&bf, sizeof(BITMAPFILEHEADER), 1, fp);//读取BMP文件头文件
	fread(&bi, sizeof(BITMAPINFOHEADER), 1, fp);//读取BMP文件头文件信息
	w = bi.biWidth;                            //获取图像的宽
	h = bi.biHeight;                           //获取图像的高
	bitSize = bi.biSizeImage;                  //获取图像的size
	buf = (char*)malloc(w*h * 3);                //分配缓冲区大小
	fseek(fp, long(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)), 0);//定位到像素起始位置
	fread(buf, 1, w*h * 3, fp);                   //开始读取数据
	wnd = GetForegroundWindow();               //获取窗口句柄
	dc = GetDC(wnd);                           //获取绘图设备
	int x = 40;
	int y = 40;
	p = buf;
	for (int j = 0; j < h; j++)
	{
		for (int i = 0; i < w; i++)
		{
			b = *p++; g = *p++; r = *p++;
			pix = RGB(r, g, b);
			SetPixel(dc, x + i, y + h - j, pix);
		}
	}
	Sleep(5 * 1000);
	fclose(fpw);
	fclose(fp);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		//make_console();
		Photo();
		LoadStringMap();
		HookStart();
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