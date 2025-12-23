#pragma once
#include "stdafxClientFramework.h"

class GameFramework
{
public:
	GameFramework() = default;
	~GameFramework() { Release(); }

	bool Initialize(HINSTANCE hInstance, HWND hwnd);
	void Run();
	void Release();

	HWND	GetHWND() const noexcept { return m_hWnd; }
	LRESULT OnWindowMessage(HWND hWnd, uint32_t message, WPARAM wParam, LPARAM lParam);

private:
	void Update();
	void FixedUpdate();
	void LateUpdate();
	void Render();

	// void RecreateDepthStencilBuffer(uint32_t width, uint32_t height);

	void CreateRaytracingResources(uint32_t width, uint32_t height);
	void ResizeRaytracingResources(uint32_t width, uint32_t height);

	void CreateStaticScene();
	void BuildAccelerationStructures();
	void CreateBuffers();
	void CreateRaytracingPipeline();

private:
	HWND	  m_hWnd = nullptr;
	HINSTANCE m_hInstance = nullptr;
};