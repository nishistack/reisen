/* $Id$ */

#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <zlib.h>

#include "sfx.h"

#include "../version.h"

#define StringText(dc,rt,str) DrawText(dc, str, -1, &rt, DT_WORDBREAK)

HINSTANCE hInst;
FILE* finst;
HFONT titlefont;
HFONT normalfont;
HFONT versionfont;
HBRUSH graybrush;
HBRUSH darkgraybrush;

uint32_t bytes = 0;
char name[257];
char setupname[512];
char welcome[512];

HWND windows[512];
int window_count = 0;
int phase = 0;

void InitWindowQueue(void){
	int i;
	for(i = 0; windows[i] != NULL; i++){
		DestroyWindow(windows[i]);
	}
	window_count = 0;
}

enum phases {
	PHASE_WELCOME = 0,
	PHASE_DIR
};

void AddWindowQueue(HWND w){
	windows[window_count++] = w;
}

void RenderPhase(int phase, HWND hWnd){
	RECT rc;

	GetClientRect(hWnd, &rc);
	InitWindowQueue();
	if(phase == PHASE_WELCOME || phase == PHASE_DIR){
		AddWindowQueue(CreateWindow("BUTTON", "Next", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, rc.right - 75 - 15 - 75 - 25, rc.bottom - 10 - 25, 75, 25, hWnd, (HMENU)GUI_SFX_NEXT, hInst, NULL));
	}
	AddWindowQueue(CreateWindow("BUTTON", "Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, rc.right - 75 - 15, rc.bottom - 10 - 25, 75, 25, hWnd, (HMENU)GUI_SFX_CANCEL, hInst, NULL));
}

void ShowBitmapSize(HWND hWnd, HDC hdc, const char* name, int x, int y, int w, int h) {
        HBITMAP hBitmap = LoadBitmap(hInst, name);
        BITMAP bmp;
        HDC hmdc;
        GetObject(hBitmap, sizeof(bmp), &bmp);
        hmdc = CreateCompatibleDC(hdc);
        SelectObject(hmdc, hBitmap);
        if(w == 0 && h == 0) {
                StretchBlt(hdc, x, y, bmp.bmWidth, bmp.bmHeight, hmdc, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
        } else {
                StretchBlt(hdc, x, y, w, h, hmdc, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
        }
        DeleteDC(hmdc);
        DeleteObject(hBitmap);
}

void AskQuit(HWND hWnd){
	int ret = MessageBox(hWnd, "Are you sure you want to quit?", "Confirm", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);

	if(ret == IDYES){
		DestroyWindow(hWnd);
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
	if(msg == WM_CLOSE){
		AskQuit(hWnd);
	}else if(msg == WM_CHAR){
		if(wp == 0x1b) AskQuit(hWnd);
	}else if(msg == WM_COMMAND){
		int trig = LOWORD(wp);
		int ev = HIWORD(wp);
		if(trig == GUI_SFX_CANCEL){
			if(ev == BN_CLICKED){
				AskQuit(hWnd);
			}
		}else if(trig == GUI_SFX_NEXT){
			if(ev == BN_CLICKED){
				phase++;
				RenderPhase(phase, hWnd);
				RedrawWindow(hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
			}
		}
	}else if(msg == WM_CREATE){
		RECT rc;

		GetClientRect(hWnd, &rc);


		titlefont = CreateFont(25, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, FF_SWISS, NULL);
		normalfont = CreateFont(15, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, FF_SWISS, NULL);
		versionfont = CreateFont(10, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, FF_MODERN, NULL);
		graybrush = CreateSolidBrush(RGB(0xc0, 0xc0, 0xc0));
		darkgraybrush = CreateSolidBrush(RGB(0xa0, 0xa0, 0xa0));

		RenderPhase(PHASE_WELCOME, hWnd);

	}else if(msg == WM_DESTROY){
		DeleteObject(titlefont);
		DeleteObject(normalfont);
		DeleteObject(versionfont);
		DeleteObject(graybrush);
		DeleteObject(darkgraybrush);
		PostQuitMessage(0);
	}else if(msg == WM_PAINT){
		RECT rc;
		PAINTSTRUCT ps;
		HDC dc;
		RECT rt;

		GetClientRect(hWnd, &rc);

		rt.left = 150 + 10;
		rt.top = 10;
		rt.right = rc.right - 10;
		rt.bottom = rc.bottom - 10;

		dc = BeginPaint(hWnd, &ps);
		SetTextColor(dc, RGB(0, 0, 0));
		ShowBitmapSize(hWnd, dc, "INSTALLER", 0, 0, 0, 0);

		if(phase == PHASE_WELCOME){
			SelectObject(dc, titlefont);
			StringText(dc, rt, welcome);
			rt.top += 25;
	
			SelectObject(dc, normalfont);
	
			StringText(dc, rt, "Setup will guide you through the installation of this application.");
			rt.top += 15;
			rt.top += 15;
	
			StringText(dc, rt, "It is recommended that you close all other applications before starting Setup. This will make it possible to update relevant system files without having to reboot your computer.");
			rt.top += 15;
			rt.top += 15;
	
			StringText(dc, rt, "Click Next to continue.");
		}else if(phase == PHASE_DIR){
			SelectObject(dc, titlefont);
			StringText(dc, rt, "Select the folder to be installed in.");
			rt.top += 25;
		}

		GetClientRect(hWnd, &rc);
		rc.left = 150;
		rc.top = rc.bottom - 75;
		FillRect(dc, &rc, graybrush);

		rc.left += 10;
		rc.top += 10;
		SelectObject(dc, versionfont);
		SetBkColor(dc, RGB(0xc0, 0xc0, 0xc0));
		SetTextColor(dc, RGB(0x80, 0x80, 0x80));
		StringText(dc, rc, "Reisen SFX " REISEN_VERSION " - http://nishi.boats/reisen");

		EndPaint(hWnd, &ps);
	}else{
		return DefWindowProc(hWnd, msg, wp, lp);
	}
	return 0;
}

BOOL InitApp(void) {
	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInst;
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "reisensfx";
	wc.hIconSm = NULL;
	return RegisterClassEx(&wc);
}

BOOL InitWindow(int nCmdShow) {
	HWND hWnd;
	RECT deskrc, rc;
	HWND hDeskWnd = GetDesktopWindow();
	GetWindowRect(hDeskWnd, &deskrc);
	windows[0] = NULL;
	hWnd = CreateWindow("reisensfx", setupname, (WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME) ^ WS_MAXIMIZEBOX, 0, 0, 600, 400, NULL, 0, hInst, NULL);

	if(!hWnd) {
		return FALSE;
	}
	GetWindowRect(hWnd, &rc);
	SetWindowPos(hWnd, HWND_TOP, (deskrc.right - (rc.right - rc.left)) / 2, (deskrc.bottom - (rc.bottom - rc.top)) / 2, rc.right - rc.left, rc.bottom - rc.top, SWP_SHOWWINDOW);
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	return TRUE;
}

int WINAPI WinMain(HINSTANCE hCurInst, HINSTANCE hPrevInst, LPSTR lpsCmdLine, int nCmdShow) {
	char path[MAX_PATH + 1];
	int len = GetModuleFileName(hCurInst, path, MAX_PATH);
	FILE* f;
	MSG msg;
	BOOL bret;
	uint8_t n;
	int i;
	phase = PHASE_WELCOME;
	hInst = hCurInst;
	name[256] = 0;
	path[len] = 0;
	finst = fopen(path, "rb");
	if(f == NULL){
		return FALSE;
	}
	fseek(finst, 0, SEEK_END);
	fseek(finst, -4-256, SEEK_CUR);
	fread(name, 1, 256, finst);
	for(i = 0; i < 4; i++){
		fread(&n, 1, 1, finst);
		bytes <<= 8;
		bytes |= n;
	}
	strcpy(setupname, name);
	strcpy(setupname + strlen(name), " Setup");
	setupname[strlen(name) + 6] = 0;

	strcpy(welcome, "Welcome to ");
	strcpy(welcome + 11, setupname);
	welcome[strlen(welcome) + 11] = 0;
	if(!InitApp()){
		return FALSE;
	}
	if(!InitWindow(nCmdShow)){
		return FALSE;
	}
	while((bret = GetMessage(&msg, NULL, 0, 0)) != 0) {
		if(bret == -1) {
			break;
		} else {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return (int)msg.wParam;
}
