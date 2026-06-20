#include "stdafxClient.h"
#include "LoginSceneControllerComponent.h"
#include "InputGlobal.h"
#include "NetworkGlobal.h"
#include "Packets/C2SPackets.h"

namespace
{
	constexpr int kLoginDialogWidth = 350;
	constexpr int kLoginDialogHeight = 250;
	constexpr int kIdEditID = 1001;
	constexpr int kPwEditID = 1002;
	constexpr int kRegisterButtonID = 1003;
	constexpr int kLoginButtonID = 1004;
	constexpr int kCancelButtonID = 1005;

	enum class LoginDialogAction
	{
		Cancel,
		Login,
		Register
	};

	struct LoginDialogResult
	{
		LoginDialogAction action = LoginDialogAction::Cancel;
		std::string id;
		std::string password;
	};

	std::string WideToUtf8(const std::wstring& text)
	{
		if (text.empty())
		{
			return {};
		}

		const int requiredSize = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
		if (requiredSize <= 1)
		{
			return {};
		}

		std::string result(static_cast<size_t>(requiredSize), '\0');
		WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, result.data(), requiredSize, nullptr, nullptr);
		result.resize(static_cast<size_t>(requiredSize - 1));
		return result;
	}

	std::wstring GetWindowTextString(HWND hwnd)
	{
		const int length = GetWindowTextLengthW(hwnd);
		if (length <= 0)
		{
			return {};
		}

		std::wstring text(static_cast<size_t>(length + 1), L'\0');
		GetWindowTextW(hwnd, text.data(), length + 1);
		text.resize(static_cast<size_t>(length));
		return text;
	}

	void FinishLoginDialog(HWND hwnd, LoginDialogResult& result, LoginDialogAction action)
	{
		HWND idEdit = GetDlgItem(hwnd, kIdEditID);
		HWND pwEdit = GetDlgItem(hwnd, kPwEditID);

		result.id = WideToUtf8(GetWindowTextString(idEdit));
		result.password = WideToUtf8(GetWindowTextString(pwEdit));
		result.action = action;

		DestroyWindow(hwnd);
	}

	LRESULT CALLBACK LoginDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		auto* result = reinterpret_cast<LoginDialogResult*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

		switch (message)
		{
		case WM_NCCREATE:
		{
			const auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
			SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
			return TRUE;
		}
		case WM_CREATE:
		{
			CreateWindowExW(0, L"STATIC", L"ID:", WS_CHILD | WS_VISIBLE, 20, 36, 45, 20, hwnd, nullptr, nullptr, nullptr);
			CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 70, 32, 200, 24, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdEditID)), nullptr, nullptr);
			CreateWindowExW(0, L"STATIC", L"PW:", WS_CHILD | WS_VISIBLE, 20, 76, 45, 20, hwnd, nullptr, nullptr, nullptr);
			CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_PASSWORD, 70, 72, 200, 24, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kPwEditID)), nullptr, nullptr);
			CreateWindowExW(0, L"BUTTON", L"Register", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 16, 132, 75, 28, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kRegisterButtonID)), nullptr, nullptr);
			CreateWindowExW(0, L"BUTTON", L"Login", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 165, 132, 60, 28, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kLoginButtonID)), nullptr, nullptr);
			CreateWindowExW(0, L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 240, 132, 60, 28, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kCancelButtonID)), nullptr, nullptr);
			return 0;
		}
		case WM_COMMAND:
		{
			if (!result)
			{
				break;
			}

			const int controlID = LOWORD(wParam);
			if (controlID == kLoginButtonID)
			{
				FinishLoginDialog(hwnd, *result, LoginDialogAction::Login);
				return 0;
			}

			if (controlID == kRegisterButtonID)
			{
				FinishLoginDialog(hwnd, *result, LoginDialogAction::Register);
				return 0;
			}

			if (controlID == kCancelButtonID)
			{
				result->action = LoginDialogAction::Cancel;
				DestroyWindow(hwnd);
				return 0;
			}
			break;
		}
		case WM_CLOSE:
			if (result)
			{
				result->action = LoginDialogAction::Cancel;
			}
			DestroyWindow(hwnd);
			return 0;
		}

		return DefWindowProcW(hwnd, message, wParam, lParam);
	}

	bool ShowLoginDialog(LoginDialogResult& result)
	{
		const wchar_t* className = L"EisenValorLoginDialog";
		HINSTANCE instance = GetModuleHandleW(nullptr);

		WNDCLASSEXW windowClass = {};
		windowClass.cbSize = sizeof(windowClass);
		windowClass.lpfnWndProc = LoginDialogProc;
		windowClass.hInstance = instance;
		windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
		windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
		windowClass.lpszClassName = className;
		RegisterClassExW(&windowClass);

		HWND owner = GetActiveWindow();
		RECT ownerRect = {};
		if (owner)
		{
			GetWindowRect(owner, &ownerRect);
		}
		else
		{
			ownerRect = {0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
		}

		const int x = ownerRect.left + ((ownerRect.right - ownerRect.left) - kLoginDialogWidth) / 2;
		const int y = ownerRect.top + ((ownerRect.bottom - ownerRect.top) - kLoginDialogHeight) / 2 + 200;

		HWND dialog = CreateWindowExW(
			WS_EX_DLGMODALFRAME,
			className,
			L"Login",
			WS_CAPTION | WS_SYSMENU | WS_POPUP,
			x,
			y,
			kLoginDialogWidth,
			kLoginDialogHeight,
			owner,
			nullptr,
			instance,
			&result
		);

		if (!dialog)
		{
			return false;
		}

		if (owner)
		{
			EnableWindow(owner, FALSE);
		}

		ShowWindow(dialog, SW_SHOW);
		UpdateWindow(dialog);

		MSG message = {};
		while (IsWindow(dialog) && GetMessageW(&message, nullptr, 0, 0) > 0)
		{
			TranslateMessage(&message);
			DispatchMessageW(&message);
		}

		if (owner)
		{
			EnableWindow(owner, TRUE);
			SetForegroundWindow(owner);
		}

		return result.action != LoginDialogAction::Cancel;
	}
}

void LoginSceneControllerComponent::OnUpdate(float deltaTime)
{
#ifdef APPLY_LOBBY_SERVER
	if (!m_firstFramePassed)
	{
		m_firstFramePassed = true;
		return;
	}

	if (!m_dialogShown)
	{
		m_dialogShown = true;

		LoginDialogResult result;
		if (!ShowLoginDialog(result))
		{
			PostQuitMessage(0);
			return;
		}

		if (result.id.empty())
		{
			return;
		}

		m_id = result.id;

		if (result.action == LoginDialogAction::Login)
		{
			DEBUG_LOG_FMT("[LoginSceneControllerComponent] Sending CL_LOGIN...\n");
			auto pb = NetBridge::C2S::Make_CL_LOGIN_PACKET(result.id, result.password);
			GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
			return;
		}

		DEBUG_LOG_FMT("[LoginSceneControllerComponent] Sending CL_SIGN_UP...\n");
		const std::string nickname = "TestNickName" + std::to_string(GetCurrentProcessId());
		auto pb = NetBridge::C2S::Make_CL_SIGN_UP_PACKET(result.id, result.password, nickname);
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}
#endif
}
