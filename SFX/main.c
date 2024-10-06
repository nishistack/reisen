/* $Id$ */

#include <windows.h>
#include <stdio.h>
#include <zlib.h>

HINSTANCE hInst;
FILE* finst;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
	if(msg == WM_CLOSE){
		int ret = MessageBox(hWnd, "Are you sure you want to quit?", "Confirm", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);

		if(ret == IDYES){
			DestroyWindow(hWnd);
		}
	}else if(msg == WM_DESTROY){
		PostQuitMessage(0);
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
	wc.hbrBackground = GetSysColorBrush(COLOR_MENU);
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
	hWnd = CreateWindow("reisensfx", "Reisen SFX", (WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME) ^ WS_MAXIMIZEBOX, 0, 0, 600, 400, NULL, 0, hInst, NULL);

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
	hInst = hCurInst;
	if(!InitApp()){
		return FALSE;
	}
	if(!InitWindow(nCmdShow)){
		return FALSE;
	}
	path[len] = 0;
	finst = fopen(path, "rb");
	if(f == NULL){
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
