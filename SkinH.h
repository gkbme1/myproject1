#ifndef SKIN_H
#define SKIN_H

	extern "C" __declspec(dllimport) int  __stdcall SkinH_Attach();
	extern "C" __declspec(dllimport) int  __stdcall SkinH_Attach_Ex(LPCTSTR strSkinFile,LPCTSTR strPassWd);
	extern "C" __declspec(dllimport) int  __stdcall SkinH_Detach();
	extern "C" __declspec(dllimport) int  __stdcall SkinH_Detach_Ex(HWND hWnd);
	extern "C" __declspec(dllimport) int  __stdcall SkinH_Attach_Ext(LPCTSTR strSkinFile,CHAR strPassWd[16],int nHue,int nSat,int nBri);
	extern "C" __declspec(dllimport) int  __stdcall SkinH_Attach_Res(LPBYTE pShe,DWORD dwSize,CHAR strPassWd[16],int nHue,int nSat,int nBri);
	extern "C" __declspec(dllimport) int  __stdcall SkinH_Alpha(HWND hWnd,int nAlpha);
	extern "C" __declspec(dllimport) int  __stdcall SkinH_Adjust_HSV(int nHue,int nSat,int nBri);
	extern "C" __declspec(dllimport) int  __stdcall SkinH_GetColor(HWND hWnd,int nX,int nY);
	extern "C" __declspec(dllimport) int  __stdcall SkinH_Map(HWND hWnd,int nType);
	extern "C" __declspec(dllimport) int  __stdcall SkinH_Aero(int bAero);
	extern "C" __declspec(dllimport) int  __stdcall SkinH_Aero_Ajust(int nAlpha, int nShwDark, int nShwSharp,int nShwSize,int nX,int nY,int nRed,int nGreen,int nBlue);
	extern "C" __declspec(dllimport) int  __stdcall SkinH_DisMove(HWND hWnd,BOOL bMove);
	extern "C" __declspec(dllimport) int  __stdcall SkinH_BGColor(HWND hWnd,int nRed, int nGreen, int nBlue);
	extern "C" __declspec(dllimport) int  __stdcall SkinH_TextColor(HWND hWnd,int nRed, int nGreen, int nBlue);

#endif