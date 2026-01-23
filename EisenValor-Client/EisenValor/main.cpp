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
#include <DxRendererGlobal.h>
#include <DxSwapChain.h>

#include "Scene/SampleScene.h"
#include "RenderPass/DxrRenderPass.h"
#include "RenderPass/CopyToBackBufferPass.h"
#include "Packets/PacketHandler.h"
#include "Packets/C2SPackets.h"
#include <TimerGlobal.h>

constexpr size_t MAX_LOADSTRING = 100;
WCHAR			 szTitle[MAX_LOADSTRING];
WCHAR			 szWindowClass[MAX_LOADSTRING];

#if _WIN32_WINNT >= 0x0A00 // Windows 10 이상
extern "C"
{
	__declspec(dllexport) extern const UINT D3D12SDKVersion = 618;
}
extern "C"
{
	__declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";
}
#endif


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
		szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, kDefaultWindowWidth, kDefaultWindowHeight, NULL,
		NULL, hInstance, NULL
	);

	if (!hWnd)
	{
		return FALSE;
	}

	if (!g_Framework->Initialize(hInstance, hWnd/*, "100.90.51.106"*//*, 7777*/))
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

	GameFramework gameFramework;
	g_Framework = &gameFramework;

	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_EISENVALOR, szWindowClass, MAX_LOADSTRING);

	if (!RegisterWindowClass(hInstance))
		return FALSE;
	if (!CreateAppWindow(hInstance, nCmdShow)) // g_Framework->Initialize(hInstance, hWnd)
		return FALSE;

	// Timer 초기화
	{
		auto& timer = GLOBAL(TimerGlobal);
		timer.SetFixedFPS(60);
		timer.SetTargetFPS(0);
	}

	// PacketHandler 등록
	{
		std::unique_ptr<NetBridge::IPacketHandler> packetHandler = std::make_unique<NetBridge::ServerPacketHandler>();
		GLOBAL(NetBridge::NetworkGlobal).SetPacketHandler(std::move(packetHandler));
		std::string id, pw;
		std::cout << "Input ID(any):";
		//std::cin >> id;
		id = "ID";
		std::cout << "\n";
		std::cout << "Input PW(any):";
		//std::cin >> pw;
		pw = "PW";

		auto pb = NetBridge::C2S::Make_CS_LOGIN_PACKET(id.c_str(), pw.c_str());
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}

	// RenderPass 등록
	{
		auto& renderer = GLOBAL(DxRendererGlobal);
		auto* swapChain = renderer.GetSwapChain();

		auto width = swapChain->GetWidth();
		auto height = swapChain->GetHeight();

		// DXR Pass 생성
		auto  dxrPass = std::make_unique<DxrRenderPass>(width, height);
		auto* outputTexture = dxrPass->GetOutputTexture();
		renderer.AddRenderPass("DXR", std::move(dxrPass));

		// CopyToBackBuffer Pass 생성
		auto copyPass = std::make_unique<CopyToBackBufferPass>(outputTexture, renderer.GetSwapChain());
		renderer.AddRenderPass("CopyToBackBuffer", std::move(copyPass));
	}

	// Scene 등록
	{
		GLOBAL(SceneGlobal).RegisterScene<SampleScene>("SampleScene");
		GLOBAL(SceneGlobal).LoadScene("SampleScene");
	}

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
	gameFramework.Release();
#ifdef _DEBUG
	FreeConsole();
#endif
	return msg.wParam;
}
