#include "stdafxClient.h"
#define _CRTDBG_MAP_ALLOC
#include "EisenValor.h"
#include "GameFramework.h"
#include "SceneGlobal.h"
// #include "Vec3.h"
#include "DxMath.h"
#include <chrono>
#include <GlobalInterfaces.h>
#include <DxCommandQueueGlobal.h>

constexpr size_t MAX_LOADSTRING = 100;
WCHAR			 szTitle[MAX_LOADSTRING];
WCHAR			 szWindowClass[MAX_LOADSTRING];

GameFramework* g_Framework = nullptr;

LRESULT CALLBACK WndProc(HWND hWnd, uint32_t msg, WPARAM wParam, LPARAM lParam)
{
	if (g_Framework)
		return g_Framework->OnWindowMessage(hWnd, msg, wParam, lParam);
}

bool RegisterWindowClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_EISENVALOR));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

bool CreateAppWindow(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd = CreateWindowW(
		szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL
	);

	if (!hWnd)
	{
		return FALSE;
	}

	if (!g_Framework->Initialize(hInstance, hWnd))
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

int APIENTRY
wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
#ifdef _DEBUG
	if (AllocConsole())
	{
		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		DWORD  mode = 0;
		GetConsoleMode(hConsole, &mode);
		mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		SetConsoleMode(hConsole, mode);

		FILE* fp;
		freopen_s(&fp, "CONOUT$", "w", stdout);
		freopen_s(&fp, "CONOUT$", "w", stderr);
		freopen_s(&fp, "CONIN$", "r", stdin);
		std::ios::sync_with_stdio();
	}

	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	// _CrtSetBreakAlloc(1739);
#endif _DEBUG

	MSG	 msg;
	bool quit = false;
	{
		GameFramework gameFramework;
		g_Framework = &gameFramework;

		LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
		LoadStringW(hInstance, IDC_EISENVALOR, szWindowClass, MAX_LOADSTRING);

		if (!RegisterWindowClass(hInstance))
			return FALSE;
		if (!CreateAppWindow(hInstance, nCmdShow))
			return FALSE;


		while (not quit)
		{
			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{ // event
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
				if (msg.message == WM_QUIT)
				{
					quit = true;
					break;
				}
			}
			if (not quit)
				gameFramework.Run();
		}
	}
	Globals::Shutdown();
#ifdef _DEBUG
	FreeConsole();
#endif
	return msg.wParam;
}
