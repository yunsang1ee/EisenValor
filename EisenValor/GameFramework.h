#pragma once
#include "Global.h"

class GameFramework
{
	GlobalRegistry globalRegistry;
public:
	GameFramework() : globalRegistry{} {}

	bool Initialize(HINSTANCE hInstance, HWND hwnd);
	void Run();
	void Release();

	HWND GetHWND() const noexcept { return m_hWnd; }

	LRESULT OnWindowMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	 
private:
	void Update();
	void LateUpdate();
	void Render();

private:
	HWND m_hWnd = nullptr;
	HINSTANCE m_hInstance = nullptr;
};