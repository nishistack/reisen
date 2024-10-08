/* $Id$ */

#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <zlib.h>

#include "sfx.h"

#include "../version.h"

#define COMPRESS 16384

#define StringText(dc,rt,str) DrawText(dc, str, -1, &rt, DT_WORDBREAK)

HWND edit;
HWND progress;
HINSTANCE hInst;
FILE* finst;
char* config;
HFONT titlefont;
HFONT normalfont;
HFONT versionfont;
HBRUSH graybrush;
HBRUSH darkgraybrush;

uint32_t bytes = 0;
uint32_t counts;
char name[257];
char setupname[512];
char welcome[512];
char* checks[512];

BOOL change = FALSE;
BOOL dirchange = TRUE;
BOOL err;
HWND windows[512];
int window_count = 0;
int gphase = 0;
char* run;
int checkcount = 0;
char instpath[MAX_PATH];

struct entry {
	char* name;
	uint32_t size;
	BOOL dir;
	BOOL skip;
};

struct entry entries[2048];

void RenderPhase(int phase, HWND hWnd);

int CALLBACK BrowseProc(HWND hWnd, UINT msg, LPARAM lp, LPARAM lp2){
	return 0;
}

char* rs_strdup(const char* a){
	char* str = malloc(strlen(a) + 1);
	memcpy(str, a, strlen(a));
	str[strlen(a)] = 0;
	return str;
}

void InitWindowQueue(void){
	int i;
	for(i = 0; windows[i] != NULL; i++){
		DestroyWindow(windows[i]);
	}
	window_count = 0;
}

enum phases {
	PHASE_WELCOME = 0,
	PHASE_DIR,
	PHASE_INSTALLING,
	PHASE_INSTALLED,
	PHASE_ERROR
};

void AddWindowQueue(HWND w){
	SendMessage(w, WM_SETFONT, (WPARAM)normalfont, TRUE);
	windows[window_count++] = w;
	windows[window_count] = NULL;
}

void ExtractProc(void* arg){
	HWND hWnd = (HWND)arg;
	int i;
	int incr = 0;
	BOOL fs = TRUE;
	char sep[MAX_PATH + 1];
	int nsep = 0;
	char cpath[MAX_PATH + 1];
	cpath[0] = 0;
	for(i = 0; instpath[i] != 0; i++){
		if(instpath[i] == '/') instpath[i] = '\\';
	}
	for(i = 0;; i++){
		if(instpath[i] == '\\' || instpath[i] == 0){
			if(fs){
				fs = FALSE;
				strcat(cpath, sep);
			}else{
				if(nsep > 0){
					strcat(cpath, "\\");
					strcat(cpath, sep);
					sep[0] = 0;

					CreateDirectory(cpath, NULL);
				}
			}
			nsep = 0;
			if(instpath[i] == 0) break;
		}else{
			sep[nsep++] = instpath[i];
			sep[nsep] = 0;
		}
	}
	incr = 0;
	fseek(finst, 0, SEEK_END);
	fseek(finst, -4-256-bytes, SEEK_CUR);
	for(i = 0; i < counts; i++){
		char destpath[MAX_PATH];
		sprintf(destpath, "%s%s", instpath, entries[i].name);
retry:
		fseek(finst, strlen(entries[i].name), SEEK_CUR);
		if(entries[i].skip){
			fseek(finst, entries[i].size, SEEK_CUR);
			fseek(finst, 4, SEEK_CUR);
			fseek(finst, 1, SEEK_CUR);
		}else if(entries[i].dir){
			fseek(finst, 4, SEEK_CUR);
			fseek(finst, 1, SEEK_CUR);
			CreateDirectory(destpath, NULL);
		}else{
			FILE* f;
			z_stream strm;
			unsigned char in[COMPRESS];
			unsigned char out[COMPRESS];
			int ret;
			uint32_t buflen = entries[i].size;
			fseek(finst, 1, SEEK_CUR);

			if(access(destpath, F_OK) == 0){
				int j;
				for(j = 0; checks[j] != NULL; j++){
					if(strcmp(checks[j], entries[i].name) == 0){
						char txt[MAX_PATH + 512];
						txt[0] = 0;
						strcat(txt, "File ");
						strcat(txt, destpath);
						strcat(txt, " already exists.\r\nOverwrite?");
						int ret = MessageBox(hWnd, txt, "Confirm", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
						if(ret == IDNO){
							goto skip;
						}	
					}
				}
			}
			f = fopen(destpath, "wb");
			if(f == NULL){
				int ret = MessageBox(hWnd, "Error opening output.\r\nRetry?", "Error", MB_YESNO | MB_ICONERROR | MB_DEFBUTTON2);
				if(ret == IDYES){
					goto retry;
				}else{
					goto itsover;
				}
			}

			strm.zalloc = Z_NULL;
			strm.zfree = Z_NULL;
			strm.opaque = Z_NULL;
			strm.avail_in = 0;
			strm.next_in = 0;
			ret = inflateInit(&strm);
			do {
				strm.avail_in = fread(in, 1, buflen < COMPRESS ? buflen : COMPRESS, finst);
				if(strm.avail_in == 0) break;
				strm.next_in = in;
				do {
					int have;
					strm.avail_out = COMPRESS;
					strm.next_out = out;
					inflate(&strm, Z_NO_FLUSH);
					have = COMPRESS - strm.avail_out;
					fwrite(out, 1, have, f);
				}while(strm.avail_out == 0);
				buflen -= buflen < COMPRESS ? buflen : COMPRESS;
			} while(ret != Z_STREAM_END);
			inflateEnd(&strm);

			fclose(f);
skip:
			fseek(finst, 4, SEEK_CUR);
			fseek(finst, 1, SEEK_CUR);
		}
		SendMessage(progress, PBM_SETPOS, (WPARAM)((double)(i + 1) / counts * 100), 0);
	}
	if(run != NULL) system(run);
	gphase = PHASE_INSTALLED;
	change = TRUE;
	_endthread();
itsover:
	gphase = PHASE_ERROR;
	change = TRUE;
	_endthread();
}

void RenderPhase(int phase, HWND hWnd){
	RECT rc;

	GetClientRect(hWnd, &rc);
	InitWindowQueue();
	if(phase == PHASE_WELCOME || phase == PHASE_DIR){
		AddWindowQueue(CreateWindow("BUTTON", "&Next", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, rc.right - 75 - 15 - 75 - 25, rc.bottom - 10 - 25, 75, 25, hWnd, (HMENU)GUI_SFX_NEXT, hInst, NULL));
	}
	if(phase == PHASE_DIR){
		edit = CreateWindow("EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER, rc.left + 150 + 10, rc.top + 10 + 25 + 10, rc.right - 150 - 10 * 2 - 50 - 10, 15, hWnd, (HMENU)GUI_SFX_DIREDIT, hInst, NULL);
		SendMessage(edit, WM_SETTEXT, 0, (LPARAM)instpath);
		AddWindowQueue(edit);
		AddWindowQueue(CreateWindow("BUTTON", "&Browse", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, rc.right - 10 - 50, rc.top + 10 + 25 + 10, 50, 15, hWnd, (HMENU)GUI_SFX_BROWSE, hInst, NULL));
	}else if(phase == PHASE_INSTALLING){
		progress = CreateWindow(PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE | PBS_SMOOTH, rc.left + 150 + 10, rc.top + 10 + 25 + 10, rc.right - 150 - 10 * 2, 15, hWnd, (HMENU)0, hInst, NULL);
		SendMessage(progress, PBM_SETPOS, (WPARAM)0, 0);
		AddWindowQueue(progress);
		_beginthread(ExtractProc, 0, hWnd);
	}
	AddWindowQueue(CreateWindow("BUTTON", (phase == PHASE_INSTALLED || phase == PHASE_ERROR) ? "&Exit" : "&Cancel", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, rc.right - 75 - 15, rc.bottom - 10 - 25, 75, 25, hWnd, (HMENU)GUI_SFX_CANCEL, hInst, NULL));
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
				if(gphase == PHASE_ERROR || gphase == PHASE_INSTALLED){
					DestroyWindow(hWnd);
				}else{
					AskQuit(hWnd);
				}
			}
		}else if(trig == GUI_SFX_NEXT){
			if(ev == BN_CLICKED){
				if(gphase == PHASE_DIR){
					SendMessage(edit, WM_GETTEXT, MAX_PATH, (LPARAM)instpath);
				}
				if(gphase == PHASE_WELCOME){
					if(dirchange){
						gphase++;
					}else{
						gphase += 2;
					}
				}else{
					gphase++;
				}
				RenderPhase(gphase, hWnd);
				RedrawWindow(hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
			}
		}else if(trig == GUI_SFX_BROWSE){
			if(ev == BN_CLICKED){
				BROWSEINFO bi;
				ITEMIDLIST* lpid;
				memset(&bi, 0, sizeof(bi));
				bi.hwndOwner = hWnd;
				bi.lpfn = BrowseProc;
				bi.ulFlags = BIF_VALIDATE;
				bi.lpszTitle = "Select the install directory";
				lpid = SHBrowseForFolder(&bi);
				if(lpid != NULL){
					SHGetPathFromIDList(lpid, instpath);
					SendMessage(edit, WM_SETTEXT, 0, (LPARAM)instpath);
				}
			}
		}
	}else if(msg == WM_CREATE){
		RECT rc;

		GetClientRect(hWnd, &rc);

		InitCommonControls();

		titlefont = CreateFont(25, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, FF_SWISS, NULL);
		normalfont = CreateFont(15, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, FF_SWISS, NULL);
		versionfont = CreateFont(10, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, FF_MODERN, NULL);
		graybrush = CreateSolidBrush(RGB(0xc0, 0xc0, 0xc0));
		darkgraybrush = CreateSolidBrush(RGB(0xa0, 0xa0, 0xa0));

		RenderPhase(PHASE_WELCOME, hWnd);

		SetTimer(hWnd, 1, 10, NULL);

	}else if(msg == WM_TIMER){
		if(change){
			change = FALSE;
			RenderPhase(gphase, hWnd);
			RedrawWindow(hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
		}
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

		if(gphase == PHASE_WELCOME){
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
			rt.top += 15;
			rt.top += 15;
	
			StringText(dc, rt, "Click Next to continue.");
		}else if(gphase == PHASE_DIR){
			SelectObject(dc, titlefont);
			StringText(dc, rt, "Select the folder to be installed in.");
			rt.top += 25;
		}else if(gphase == PHASE_INSTALLING){
			SelectObject(dc, titlefont);
			StringText(dc, rt, "Installing");
			rt.top += 25;
		}else if(gphase == PHASE_ERROR){
			SelectObject(dc, titlefont);
			StringText(dc, rt, "Error");
			rt.top += 25;
		}else if(gphase == PHASE_INSTALLED){
			SelectObject(dc, titlefont);
			StringText(dc, rt, "Installation successful");
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
	wc.hIcon = LoadIcon(hInst, "REISEN");
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "reisensfx";
	wc.hIconSm = LoadIcon(hInst, "REISEN");
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
	MSG msg;
	BOOL bret;
	uint8_t n;
	int i;
	uint32_t bbytes;
	uint32_t entbytes;
	uint32_t incr;
	BOOL first = TRUE;
	run = NULL;
	config = NULL;
	instpath[0] = 'C';
	instpath[1] = ':';
	instpath[2] = '\\';
	instpath[3] = 0;
	gphase = PHASE_WELCOME;
	hInst = hCurInst;
	name[256] = 0;
	path[len] = 0;
	finst = fopen(path, "rb");
	if(finst == NULL){
		printf("panic: cannot read %s\n", path);
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
	strcat(instpath, name);
	counts = 0;
	checks[0] = NULL;
loop:
	bbytes = bytes;
	fseek(finst, 0, SEEK_END);
	fseek(finst, -4-256, SEEK_CUR);
	incr = 0;
	while(1){
		uint8_t fnlen;
		uint8_t attr;
		char* fnnam;
		fseek(finst, -1, SEEK_CUR);
		fread(&attr, 1, 1, finst);
		fseek(finst, -1-4, SEEK_CUR);
		for(i = 0; i < 4; i++){
			fread(&n, 1, 1, finst);
			entbytes <<= 8;
			entbytes |= n;
		}
		fflush(stdout);
		if(attr == 1){
			fseek(finst, -4-entbytes-1, SEEK_CUR);
			fread(&fnlen, 1, 1, finst);
			fseek(finst, -1-fnlen, SEEK_CUR);
			fnnam = malloc(fnlen + 1);
			fnnam[fnlen] = 0;
			fread(fnnam, fnlen, 1, finst);
			fseek(finst, -fnlen, SEEK_CUR);

			bytes -= fnlen + 1 + entbytes + 4 + 1;

			if(first){
				free(fnnam);
			}else{
				int i;
				entries[counts - incr - 1].dir = FALSE;
				entries[counts - incr - 1].skip = FALSE;
				entries[counts - incr - 1].name = fnnam;
				entries[counts - incr - 1].size = entbytes;
				for(i = 0; fnnam[i] != 0; i++){
					if(fnnam[i] == '/') fnnam[i] = '\\';
				}
				incr++;
			}
		}else if(attr == 0){
			fnnam = malloc(entbytes + 1);
			fnnam[entbytes] = 0;
			fseek(finst, -4-entbytes, SEEK_CUR);
			fread(fnnam, 1, entbytes, finst);
			fseek(finst, -entbytes, SEEK_CUR);
			bytes -= entbytes + 4 + 1;

			if(first){
				free(fnnam);
			}else{
				entries[counts - incr - 1].dir = TRUE;
				entries[counts - incr - 1].skip = FALSE;
				entries[counts - incr - 1].name = fnnam;
				for(i = 0; fnnam[i] != 0; i++){
					if(fnnam[i] == '/') fnnam[i] = '\\';
				}
				incr++;
			}
		}else if(attr == 2){
			config = malloc(entbytes + 1);
			config[entbytes] = 0;
			fseek(finst, -4-entbytes, SEEK_CUR);
			fread(config, 1, entbytes, finst);
			fseek(finst, -entbytes, SEEK_CUR);
			bytes -= entbytes + 4 + 1;

			if(first){
				free(config);
			}else{
				entries[counts - incr - 1].skip = TRUE;
				entries[counts - incr - 1].size = entbytes;
				entries[counts - incr - 1].name = "";
				incr++;
			}
		}
		if(first){
			counts++;
		}
		if(bytes == 0) break;
	}
	bytes = bbytes;
	if(first){
		first = FALSE;
		goto loop;
	}

	if(config != NULL){
		int i;
		int incr = 0;
		for(i = 0;; i++){
			if(config[i] == '\n' || config[i] == 0){
				char oldc = config[i];
				char* line = config + incr;
				config[i] = 0;

				if(strlen(line) > 0 && line[0] != '#'){
					int j;
					for(j = 0;; j++){
						if(line[j] == ' ' || line[j] == '\t' || line[j] == 0){
							BOOL hasarg = line[j] != 0;
							char* arg = NULL;
							char oldc2 = line[j];
							line[j] = 0;
							if(hasarg){
								j++;
								for(; line[j] != 0 && (line[j] == '\t' || line[j] == ' '); j++);
								arg = line + j;
							}
							if(strcmp(line, "DefaultDirectory") == 0 && hasarg){
								strcpy(instpath, arg);
								instpath[strlen(arg)] = 0;
							}else if(strcmp(line, "DirectoryUnchangable") == 0){
								dirchange = FALSE;
							}else if(strcmp(line, "CheckOverwrite") == 0 && hasarg){
								int k;
								checks[checkcount] = rs_strdup(arg);
								checks[checkcount + 1] = NULL;
								for(k = 0; checks[checkcount][k] != 0; k++){
									if(checks[checkcount][k] == '/') checks[checkcount][k] = '\\';
								}
								checkcount++;
							}else if(strcmp(line, "Run") == 0 && hasarg){
								run = rs_strdup(arg);
							}
							line[j] = oldc2;
							break;
						}
					}
				}

				config[i] = oldc;
				incr = i + 1;
				if(oldc == 0) break;
			}
		}
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
