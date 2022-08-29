#include "wm.h"

#include <stddef.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static bool s_quit = false;

static LRESULT CALLBACK _window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE:
		s_quit = true;
		break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

wm_window_t* wm_create()
{
	WNDCLASS wc =
	{
		.lpfnWndProc = _window_proc,
		.hInstance = GetModuleHandle(NULL),
		.lpszClassName = L"ga2022 window class",
	};
	RegisterClass(&wc);

	HWND hwnd = CreateWindowEx(
		0,
		wc.lpszClassName,
		L"GA 2022",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		wc.hInstance,
		NULL);

	ShowWindow(hwnd, TRUE);

	return (wm_window_t*)hwnd;
}

bool wm_pump(wm_window_t* window)
{
	MSG msg = { 0 };
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return s_quit;
}

void wm_destroy(wm_window_t* window)
{
	DestroyWindow(window);
}
