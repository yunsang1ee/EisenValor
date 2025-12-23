#pragma once
#include "stdafxClientFramework.h"
#include "DxSwapChain.h"
#include "DxCommon.h"
#include "DxFeatureCaps.h"
#include "DxFrameResource.h"
#include "DxTLAS.h"
#include "DxRtPipelineState.h"
#include "DxRtShaderTable.h"
#include <memory>
#include "DxTexture.h"
#include "DxBuffer.h"
#include "DxDescriptorHeapGlobal.h"

class GameFramework
{
public:
	GameFramework() = default;
	~GameFramework() { Release(); }

	bool Initialize(HINSTANCE hInstance, HWND hwnd);
	void Run();
	void Release();

	HWND		 GetHWND() const noexcept { return m_hWnd; }
	DxSwapChain* GetSwapChain() const { return m_swapChain.get(); }

	uint32_t GetWidth() const { return m_swapChain->GetWidth(); }
	uint32_t GetHeight() const { return m_swapChain->GetHeight(); }

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

	void RenderDXR();

	void UpdateCamera(float deltaTime);
	void UpdateCameraVectors();


private:
	DxFeatureCaps m_featureCaps;
	// 새로 추가할 멤버들 25.07.19
	std::unique_ptr<DxSwapChain> m_swapChain;
	ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
	uint32_t					 m_rtvDescriptorSize = 0;

	static constexpr uint32_t								  kFrameCount = 3;
	std::array<std::unique_ptr<DxFrameResource>, kFrameCount> m_frameResources;
	uint32_t												  m_currentFrameIndex = 0;

	DX::XMFLOAT3 m_cameraPosition = {0.0f, 5.0f, -10.0f};
	DX::XMFLOAT3 m_cameraForward = {0.0f, 0.0f, 1.0f};
	DX::XMFLOAT3 m_cameraRight = {1.0f, 0.0f, 0.0f};
	DX::XMFLOAT3 m_cameraUp = {0.0f, 1.0f, 0.0f};

	float m_cameraYaw = 0.0f;
	float m_cameraPitch = 0.0f;
	float m_cameraSpeed = 2.0f; // 이동 속도
	float m_mouseSensitivity = 0.1f;

	int	 m_lastMouseX = 0;
	int	 m_lastMouseY = 0;
	bool m_firstMouse = true;
	bool m_cameraEnabled = false; // 우클릭시 카메라 조작 활성화

private:
	HWND	  m_hWnd = nullptr;
	HINSTANCE m_hInstance = nullptr;
};