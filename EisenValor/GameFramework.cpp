#include "stdafx.h"
#include "resource.h"
#include "GameFramework.h"
#include "Global.h"
#include "InputSystem.h"
#include "Timer.h"

bool GameFramework::Initialize(HINSTANCE hInstance, HWND hwnd)
{
	m_hInstance = hInstance;
	m_hWnd = hwnd;

	globalRegistry.Register<IInputGlobal>(InputGlobal::Create());
	globalRegistry.Register<ITimerGlobal>(TimerGlobal::Create());

	globalRegistry.Get<IInputGlobal>();
	globalRegistry.Get<ITimerGlobal>();
	return true;
}

void GameFramework::Run()
{
	auto& input = globalRegistry.Get<IInputGlobal>();
	input.BeforeUpdate();
	
	//FixedUpdate();
	Update();
	LateUpdate();

	Render();

	input.AfterUpdate();
}

void GameFramework::Release()
{
}

LRESULT GameFramework::OnWindowMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	WORD keyflags;
	bool isPressed;
	bool isUp;
	InputCode code;

	switch (message)
	{
	case WM_SYSCOMMAND:
		if (wParam == SC_KEYMENU)
			return 0;
		break;
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
			globalRegistry.Get<IInputGlobal>().OnInputState(code, isPressed, isUp);
	}
		break;
	case WM_MOUSEMOVE:
		globalRegistry.Get<IInputGlobal>().OnMouseMove(LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_LBUTTONDOWN:
	{
		globalRegistry.Get<IInputGlobal>().OnInputState(VK_LBUTTON, FALSE, FALSE);
		break;
	}
	case WM_RBUTTONDOWN:
	{
		globalRegistry.Get<IInputGlobal>().OnInputState(VK_RBUTTON, FALSE, FALSE);
		break;
	}
	case WM_LBUTTONUP:
	{
		globalRegistry.Get<IInputGlobal>().OnInputState(VK_LBUTTON, FALSE, TRUE);
		break;
	}
	case WM_RBUTTONUP:
	{
		globalRegistry.Get<IInputGlobal>().OnInputState(VK_RBUTTON, FALSE, TRUE);
		break;
	}
	case WM_MOUSEWHEEL:
		globalRegistry.Get<IInputGlobal>().OnWheelScroll(GET_WHEEL_DELTA_WPARAM(wParam));
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

void GameFramework::Update()
{
	if (globalRegistry.Get<IInputGlobal>().GetInputDown(VK_ESCAPE))
	{
		DEBUG_LOG_FMT("close");
		::PostQuitMessage(0);
	}
}

void GameFramework::LateUpdate()
{
}

void GameFramework::Render()
{
}
