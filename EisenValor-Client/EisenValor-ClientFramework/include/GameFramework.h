#pragma once
#include "stdafxClientFramework.h"
#include <optional>

class GameFramework
{
public:
	GameFramework() = default;
	~GameFramework() = default;

	bool Initialize(
		HINSTANCE hInstance, HWND hwnd, std::string_view serverAddress, uint16_t serverPort
	);
	void Run();
	void Release();

	HWND	GetHWND() const noexcept { return m_hWnd; }
	LRESULT OnWindowMessage(HWND hWnd, uint32_t message, WPARAM wParam, LPARAM lParam);

private:
	void Update(float delta);
	void FixedUpdate();
	void LateUpdate(float delta);
	void Render();
	void ApplyResize(uint32_t width, uint32_t height);
	void MarkRectTransformsDirty();

private:
	struct ResizeExtent
	{
		uint32_t width = 0;
		uint32_t height = 0;
	};

	HWND	  m_hWnd = nullptr;
	HINSTANCE m_hInstance = nullptr;
	bool	  m_released = false;

	bool						m_inSizeMove = false;
	std::optional<ResizeExtent> m_pendingResize;
};
