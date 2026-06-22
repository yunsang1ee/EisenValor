#include "stdafxClientFramework.h"
#include "GameFramework.h"
#include "GlobalInterfaces.h"
#include "DxDebugGlobal.h"
#include "SceneGlobal.h"
#include "DxCommandQueueGlobal.h"
#include "DxRendererGlobal.h"
#include "DxSwapChain.h"
#include "InputGlobal.h"
#include "RectTransformComponent.h"
#include "TimerGlobal.h"
#include "ResourceGlobal.h"
#include "UIGlobal.h"
#include "PixProfiler.h"

#define SERVER

bool GameFramework::Initialize(HINSTANCE hInstance, HWND hwnd, std::string_view serverAddress, uint16_t serverPort)
{
#ifdef SERVER
	#ifdef APPLY_LOBBY_SERVER
	if (false == GLOBAL(NetBridge::NetworkGlobal).Init(serverAddress, serverPort))
	return false;
	(void)serverAddress;
	(void)serverPort;
	#endif

#endif

	m_hInstance = hInstance;
	m_hWnd = hwnd;

	Globals::Initialize(hwnd);

	RECT clientRect;
	GetClientRect(m_hWnd, &clientRect);
	uint32_t width = clientRect.right - clientRect.left;
	uint32_t height = clientRect.bottom - clientRect.top;

	auto& renderer = GLOBAL(DxRendererGlobal);
	renderer.CreateSwapChain(m_hWnd, width, height);

	DEBUG_LOG_FMT("[GameFramework] Initialized: {}x{}\n", width, height);
	return true;
}

void GameFramework::Run()
{
	PixScopedCpuEvent frameEvent(L"Frame");

#ifdef SERVER
	{
		PixScopedCpuEvent event(L"Network.ProcessIO");
		GLOBAL(NetBridge::NetworkGlobal).ProcessIO();
	}
#endif

	auto& input = GLOBAL(InputGlobal);
	{
		PixScopedCpuEvent event(L"Input.BeforeUpdate");
		input.BeforeUpdate();
	}

	bool isFocused = (GetForegroundWindow() == m_hWnd);
	input.SetWindowFocused(isFocused);

	static float runTime{};
	float		 dt = 0.0f;
	{
		PixScopedCpuEvent event(L"Timer.Update");
		dt = GLOBAL(TimerGlobal).Update();
	}

	static float timeElapsed = 0.0f;
	static int	 frameCount = 0;

	timeElapsed += dt;
	frameCount++;

	if (timeElapsed >= 1.0f)
	{
		const float fps = static_cast<float>(frameCount) / timeElapsed;
		const float ms = (timeElapsed * 1000.0f) / static_cast<float>(frameCount);

		// DEBUG_LOG_FMT("[GameFramework] FPS: {:.2f} ({:.2f} ms)\n", fps, ms);

		std::string windowTitle = std::format("EisenValor (FPS: {:.0f})", fps);
		SetWindowTextA(m_hWnd, windowTitle.c_str());

		timeElapsed = 0.0f;
		frameCount = 0;
	}

	// BeginFrame -> FixedUpdate -> Update -> LateUpate -> Render -> EndFrame -> AfterUpdate

	GLOBAL(SceneGlobal).OnBeginFrame();

	while (GLOBAL(TimerGlobal).ShouldFixedUpdate())
	{
		FixedUpdate();
	}

	Update(dt);

	LateUpdate(dt);

	Render();

	GLOBAL(SceneGlobal).OnEndFrame();
	GLOBAL(InputGlobal).AfterUpdate();

#ifdef _DEBUG
	GLOBAL(DxDebugGlobal).PrintDebugMessages();
#endif
}

void GameFramework::Release()
{
	if (m_released)
		return;
	m_released = true;

	auto* queue = GLOBAL(DxGfxCommandQueueGlobal).GetQueue();
	if (!queue)
		return;

	DEBUG_LOG_FMT("[GameFramework] Releasing resources...\n");
	GLOBAL(DxGfxCommandQueueGlobal).WaitForIdle();
	DEBUG_LOG_FMT("[GameFramework] Resource release complete\n");

	Globals::Shutdown();
}

LRESULT GameFramework::OnWindowMessage(HWND hWnd, uint32_t message, WPARAM wParam, LPARAM lParam)
{
	WORD	  keyflags;
	bool	  isPressed;
	bool	  isUp;
	InputCode code;

	switch (message)
	{
	case WM_ACTIVATE:
	case WM_ACTIVATEAPP:
		GLOBAL(InputGlobal).SetWindowFocused(WA_ACTIVE == LOWORD(wParam));
		break;

	case WM_SETFOCUS:
		GLOBAL(InputGlobal).SetWindowFocused(true);
		break;
	case WM_KILLFOCUS:
		GLOBAL(InputGlobal).SetWindowFocused(false);
		break;

	case WM_SYSCOMMAND:
		if (wParam == SC_KEYMENU)
			return 0;
		break;

	case WM_SIZE:
	{
		if (wParam == SIZE_MINIMIZED)
			break;

		const uint32_t width = static_cast<uint32_t>(LOWORD(lParam));
		const uint32_t height = static_cast<uint32_t>(HIWORD(lParam));

		auto& renderer = GLOBAL(DxRendererGlobal);
		if (renderer.GetSwapChain())
		{
			renderer.OnResize(width, height);
		}
		GLOBAL(InputGlobal).OnResize(width, height);

		if (auto* scene = GLOBAL(SceneGlobal).GetActiveScene())
		{
			if (auto* rectStorage = scene->GetStorage<RectTransformComponent>())
			{
				for (auto& rectTransform : rectStorage->GetList())
				{
					rectTransform.MarkDirty();
				}
			}
		}

		DEBUG_LOG_FMT("[GameFramework] Window resized: {}x{}\n", width, height);
		break;
	}

	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYUP:
	{
		keyflags = HIWORD(lParam);
		isPressed = (keyflags & KF_REPEAT) == KF_REPEAT;
		isUp = (keyflags & KF_UP) == KF_UP;
		code = static_cast<InputCode>(wParam);
		if (code)
			GLOBAL(InputGlobal).OnInputState(code, isPressed, isUp);
	}
	break;

	case WM_MOUSEMOVE:
		GLOBAL(InputGlobal).OnMouseMove(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_LBUTTONDOWN:
		GLOBAL(InputGlobal).OnInputState(VK_LBUTTON, FALSE, FALSE);
		break;

	case WM_RBUTTONDOWN:
		GLOBAL(InputGlobal).OnInputState(VK_RBUTTON, FALSE, FALSE);
		break;

	case WM_MBUTTONDOWN:
		GLOBAL(InputGlobal).OnInputState(VK_MBUTTON, FALSE, FALSE);
		break;

	case WM_LBUTTONUP:
		GLOBAL(InputGlobal).OnInputState(VK_LBUTTON, FALSE, TRUE);
		break;

	case WM_RBUTTONUP:
		GLOBAL(InputGlobal).OnInputState(VK_RBUTTON, FALSE, TRUE);
		break;

	case WM_MBUTTONUP:
		GLOBAL(InputGlobal).OnInputState(VK_MBUTTON, FALSE, TRUE);
		break;

	case WM_MOUSEWHEEL:
		GLOBAL(InputGlobal).OnWheelScroll(GET_WHEEL_DELTA_WPARAM(wParam));
		break;

	case WM_ENTERSIZEMOVE:
		SetTimer(m_hWnd, 1, 1, NULL);
		break;

	case WM_EXITSIZEMOVE:
		KillTimer(m_hWnd, 1);
		break;

	case WM_TIMER:
		if (wParam == 1)
		{
			Run();
		}
		break;

	case WM_DESTROY:
		DEBUG_LOG_FMT("Window destroyed. Initiating application shutdown.\n");
		PostQuitMessage(0);
		break;

	default:
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

void GameFramework::Update(float delta)
{
	PixScopedCpuEvent event(L"Scene.Update");

	auto& input = GLOBAL(InputGlobal);
	if (input.GetInputDown(VK_ESCAPE))
	{
		DEBUG_LOG_FMT("close\n");
		::DestroyWindow(m_hWnd);
	}
	if (input.GetInputDown(VK_F5))
	{
		// FUTURE: Runtime Shader Compilation
	}

	//if (input.GetInputDown(VK_F9))
	//{
	//	GLOBAL(ResourceGlobal).DumpLoadedMaterials();
	//}

	if (auto* swapChain = GLOBAL(DxRendererGlobal).GetSwapChain()) [[likely]]
	{
		if (input.GetInputDown(VK_F11))
		{
			swapChain->ToggleBorderlessFullscreen();
		}
		if (input.GetInput(VK_MENU) && input.GetInputDown(VK_RETURN))
		{
			swapChain->ToggleFullscreen();
		}
	}

	GLOBAL(UIGlobal).Update(delta);
	GLOBAL(SceneGlobal).OnUpdate(delta);
}

void GameFramework::FixedUpdate()
{
	PixScopedCpuEvent event(L"Scene.FixedUpdate");
	GLOBAL(SceneGlobal).OnFixedUpdate(GLOBAL(TimerGlobal).GetFixedDeltaTime());
}

void GameFramework::LateUpdate(float delta)
{
	PixScopedCpuEvent event(L"Scene.LateUpdate");
	GLOBAL(SceneGlobal).OnLateUpdate(delta);
}

void GameFramework::Render()
{
	PixScopedCpuEvent event(L"Render");

	auto& scene = GLOBAL(SceneGlobal);
	auto& renderer = GLOBAL(DxRendererGlobal);

	{
		PixScopedCpuEvent beginFrameEvent(L"Render.BeginFrame");
		renderer.BeginFrame();
	}

	{
		PixScopedCpuEvent resourceEvent(L"Render.ResourceUpload");
		GLOBAL(ResourceGlobal).ProcessPendingLoads();
	}

	{
		PixScopedCpuEvent rendererEvent(L"Render.RenderPasses");
		renderer.Render(scene.GetActiveScene());
	}
	{
		PixScopedCpuEvent endFrameEvent(L"Render.EndFrame");
		renderer.EndFrame();
	}

	{
		PixScopedCpuEvent reloadEvent(L"Resource.CheckForReload");
		GLOBAL(ResourceGlobal).CheckForReload();
	}
}
