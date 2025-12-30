#pragma once
#include "stdafxClientFramework.h"

class GameFramework
{
public:
	GameFramework() = default;
	~GameFramework() = default;

	bool Initialize(HINSTANCE hInstance, HWND hwnd);
	void Run();
	void Release();

	HWND	GetHWND() const noexcept { return m_hWnd; }
	LRESULT OnWindowMessage(HWND hWnd, uint32_t message, WPARAM wParam, LPARAM lParam);

private:
	void Update(float delta);
	void FixedUpdate();
	void LateUpdate(float delta);
	void Render();

private:
	HWND	  m_hWnd = nullptr;
	HINSTANCE m_hInstance = nullptr;
	bool	  m_released = false;
};
