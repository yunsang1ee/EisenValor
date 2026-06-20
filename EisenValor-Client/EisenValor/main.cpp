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

// Scene
#include "Scene/WorldScene.h"
#include "Scene/StartScene.h"
#include "Scene/LoginScene.h"
#include "Scene/LobbyScene.h"
#include "Scene/RoomScene.h"
#include "Scene/LoadingScene.h"

#include "RenderPass/SkinningPass.h"
#include "RenderPass/DxrRenderPass.h"
#include "RenderPass/HdrResolvePass.h"
#include "RenderPass/UIRenderPass.h"

#include "UIGlobal.h"
#include "ResourceGlobal.h"
#include "Component/FSM/StatePool.h"

#include "Packets/PacketHandler.h"
#include "Packets/C2SPackets.h"
#include "Network/LobbyServerSession.h"
#include "Network/GameServerSession.h"
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
	try
	{
		if (g_Framework)
		{
			return g_Framework->OnWindowMessage(hWnd, msg, wParam, lParam);
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	catch (const std::exception& e)
	{
		DEBUG_LOG_FMT("[WndProc][ERROR] Exception in message handler: {}\n", e.what());

		if (g_Framework)
		{
			// TODO: g_Framework->RequestShutdown();
		}

		return 0;
	}
	catch (...)
	{
		DEBUG_LOG_FMT("[WndProc][ERROR] Unknown exception in message handler\n");
		return 0;
	}
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
		szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, Variable::kDefaultWindowWidth,
		Variable::kDefaultWindowHeight, NULL, NULL, hInstance, NULL
	);

	if (!hWnd)
	{
		return FALSE;
	}

	std::string_view ipAddress{"127.0.0.1"};
	
	#ifdef APPLY_LOBBY_SERVER
		const uint16 port{G_LOBBY_SERVER_PORT};	// lobby port
		GLOBAL(NetBridge::NetworkGlobal).SetLobbySession(std::make_unique<NetBridge::LobbyServerSession>());
	#else
		const uint16 port{G_GAME_SERVER_PORT};	// game server 1st worldThread port
	#endif

	GLOBAL(NetBridge::NetworkGlobal).SetGameSession(std::make_unique<NetBridge::GameServerSession>());

	if (!g_Framework->Initialize(hInstance, hWnd, ipAddress, port))
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
#if defined(_DEBUG) || defined(ENABLE_DEBUG_LOG)
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
#endif

#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	// _CrtSetBreakAlloc(1739);
#endif

	MSG	 msg;
	bool quit = false;

	try
	{
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

		// RenderPass 등록
		{
			auto& renderer = GLOBAL(DxRendererGlobal);
			auto* swapChain = renderer.GetSwapChain();

			auto width = swapChain->GetWidth();
			auto height = swapChain->GetHeight();

			// Skinning Pass 생성
			auto  skinningPass = std::make_unique<SkinningPass>();
			renderer.AddRenderPass("Skinning", std::move(skinningPass), RenderPassPriority::High);

			// DXR Pass 생성
			auto  dxrPass = std::make_unique<DxrRenderPass>(width, height);
			renderer.AddRenderPass("DXR", std::move(dxrPass));

			// HDR Resolve Pass 생성
			auto hdrResolvePass = std::make_unique<HdrResolvePass>(swapChain);
			renderer.AddRenderPass("HdrResolve", std::move(hdrResolvePass));

			// UI Pass 생성
			auto uiPass = std::make_unique<UIRenderPass>();
			renderer.AddRenderPass("UI", std::move(uiPass), RenderPassPriority::UI);
		}

		// 에셋 레지스트리 로드
		GLOBAL(ResourceGlobal).LoadRegistry("Resource\\AssetRegistry.evreg");
		GLOBAL(UIGlobal).Initialize();

		// FSM StatePool 초기화
		StatePool::Initialize();

	// Scene 등록
	{
		GLOBAL(SceneGlobal).RegisterScene<StartScene>("StartScene");
		GLOBAL(SceneGlobal).RegisterScene<LoginScene>("LoginScene");
		GLOBAL(SceneGlobal).RegisterScene<WorldScene>("WorldScene");
		GLOBAL(SceneGlobal).RegisterScene<LobbyScene>("LobbyScene");
		GLOBAL(SceneGlobal).RegisterScene<RoomScene>("RoomScene");
		GLOBAL(SceneGlobal).RegisterScene<LoadingScene>("LoadingScene");

		GLOBAL(SceneGlobal).LoadScene("StartScene");
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
#if defined(_DEBUG) || defined(ENABLE_DEBUG_LOG)
		FreeConsole();
#endif
		return msg.wParam;
	}
	catch (const HrException& e)
	{
		const std::string errorMsg = std::format(
			"DirectX Error at {}({})\n{}\nError Code: {:#x}", e.GetFile(), e.GetLine(), e.what(),
			static_cast<uint32_t>(e.GetErrorCode())
		);

		DEBUG_LOG_FMT("[FATAL][D3D] {}\n", errorMsg);

		MessageBoxA(nullptr, errorMsg.c_str(), "EisenValor - Fatal Error", MB_ICONERROR | MB_OK);

		// NOTE: CreateMiniDump(e);

		return -1;
	}
	catch (const std::exception& e)
	{
		DEBUG_LOG_FMT("[FATAL][STD] {}\n", e.what());
		MessageBoxA(nullptr, e.what(), "EisenValor - Fatal Error", MB_ICONERROR | MB_OK);
		return -1;
	}
	catch (...)
	{
		DEBUG_LOG_FMT("[FATAL] Unknown exception\n");
		MessageBoxA(nullptr, "Unknown critical error occurred", "EisenValor - Fatal Error", MB_ICONERROR | MB_OK);
		return -1;
	}
}
