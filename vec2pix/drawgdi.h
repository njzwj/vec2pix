#ifndef DRAWGDI_H_
#define DRAWGDI_H_

#include <Windows.h>
#include <stdio.h>

static uint16_t gdi_w, gdi_h;
static HINSTANCE hInstance;
static HWND screen_handle = NULL;
static HDC screen_dc = NULL;
static HBITMAP screen_hb = NULL;
static HBITMAP screen_ob = NULL;
int exit_status;

LRESULT WindowEventListener(HWND window, unsigned int msg, WPARAM wp, LPARAM lp);
void DispatchEvent();
void UpdateScreen();

void InitWindow(uint16_t w, uint16_t h, const wchar_t* window_name, uint8_t **scr_buf)
{
	WNDCLASSEX wc = { };
	BITMAPINFO bi = { {sizeof(BITMAPINFOHEADER), (long)w, (long)(-h), 1, 24, BI_RGB, 
		w * h * 3, 0, 0, 0, 0} };
	HDC hdc;
	LPVOID ptr;

	gdi_w = w;
	gdi_h = h;

	hInstance = GetModuleHandle(NULL);
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_BYTEALIGNCLIENT;
	wc.lpfnWndProc = (WNDPROC)WindowEventListener;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = HBRUSH(COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = L"MYWINDOW";
	wc.hIconSm = LoadIcon(0, IDI_APPLICATION);
	if (!RegisterClassEx(&wc)) return;
	screen_handle = CreateWindowEx(0, L"MYWINDOW", window_name, 
		WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU, // prevent resizing
		CW_USEDEFAULT, CW_USEDEFAULT, w, h + 32,
		0, 0, hInstance, 0);
	if (screen_handle == NULL) return;
	exit_status = 0;

	hdc = GetDC(screen_handle);
	screen_dc = CreateCompatibleDC(hdc);
	ReleaseDC(screen_handle, hdc);

	screen_hb = CreateDIBSection(screen_dc, &bi, DIB_RGB_COLORS, &ptr, 0, 0);
	if (screen_hb == NULL) return;

	screen_ob = (HBITMAP)SelectObject(screen_dc, screen_hb);
	*scr_buf = (uint8_t *)ptr;
	memset(*scr_buf, 0, w * h * 3);

	ShowWindow(screen_handle, SW_SHOWDEFAULT);
	DispatchEvent();
}

LRESULT WindowEventListener(HWND window, unsigned int msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_CLOSE:
		exit_status = 1;
		break;
	case WM_KEYDOWN:
		printf("KEYDOWN");
		break;
	case WM_KEYUP:
		printf("KEYUP");
		break;
	default:
		return DefWindowProc(window, msg, wp, lp);
	}
	return 0L;
}

void DispatchEvent()
{
	MSG msg;
	for (;;)
	{
		if (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) break;
		if (!GetMessage(&msg, NULL, 0, 0)) break;
		DispatchMessage(&msg);
	}
}

void UpdateScreen()
{
	HDC hDC = GetDC(screen_handle);
	BitBlt(hDC, 0, 0, gdi_w, gdi_h, screen_dc, 0, 0, SRCCOPY);
	ReleaseDC(screen_handle, hDC);
	DispatchEvent();
}

#endif
