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

//// 인스턴싱을 위한 구조체
// struct InstanceData
//{
//	DirectX::XMFLOAT4X4 world; // 각 인스턴스의 월드 행렬
//	DirectX::XMFLOAT4 color;   // 각 인스턴스의 색상
// };
//
// struct PerFrameData
//{
//	DirectX::XMFLOAT4X4 view;       // 뷰 행렬
//	DirectX::XMFLOAT4X4 projection; // 프로젝션 행렬
// };

class Actor;

struct PBRMaterial
{
	DirectX::XMFLOAT3 albedo;
	float			  metallic;
	float			  roughness;
	DirectX::XMFLOAT3 emissive;
	float			  emissiveStrength;
};

class GameFramework
{
public:
	GameFramework() = default;
	~GameFramework() = default;

	bool Initialize(HINSTANCE hInstance, HWND hwnd);
	void Run();
	void Release();

	HWND GetHWND() const noexcept { return m_hWnd; }

	LRESULT OnWindowMessage(HWND hWnd, uint32_t message, WPARAM wParam, LPARAM lParam);

private:
	void Update();
	void FixedUpdate();
	void LateUpdate();
	void Render();

	//void RecreateDepthStencilBuffer(uint32_t width, uint32_t height);

	void CreateRaytracingResources(uint32_t width, uint32_t height);
	void ResizeRaytracingResources(uint32_t width, uint32_t height);

	void CreateStaticScene();
	void BuildAccelerationStructures();
	void CreateRaytracingPipeline();
	void CreateMaterialBuffer();

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

	ComPtr<ID3D12Resource> m_raytracingOutput;
	uint32_t			   m_raytracingOutputUAVIndex = ~0u;

	std::unique_ptr<DxRtPipelineState>	m_rtPipeline;
	std::unique_ptr<DxRtShaderTable>	m_shaderTable;
	std::unique_ptr<DxTLAS>				m_tlas;
	std::vector<std::unique_ptr<Actor>> m_sceneActors;

	ComPtr<ID3D12Resource>	 m_materialBuffer;
	uint32_t				 m_materialBufferSRVIndex = ~0u;
	std::vector<PBRMaterial> m_materials;

	DirectX::XMFLOAT3 m_cameraPosition = {0.0f, 5.0f, -10.0f};
	DirectX::XMFLOAT3 m_cameraForward = {0.0f, 0.0f, 1.0f};
	DirectX::XMFLOAT3 m_cameraRight = {1.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT3 m_cameraUp = {0.0f, 1.0f, 0.0f};

	float m_cameraYaw = -90.0f;
	float m_cameraPitch = 0.0f;
	float m_cameraSpeed = 10.0f; // 이동 속도
	float m_mouseSensitivity = 0.1f;

	int	 m_lastMouseX = 0;
	int	 m_lastMouseY = 0;
	bool m_firstMouse = true;
	bool m_cameraEnabled = true; // 우클릭시 카메라 조작 활성화


	//// 깊이 버퍼 관련 멤버 추가
	// ComPtr<ID3D12Resource>		 m_depthStencilBuffer;
	// ComPtr<ID3D12DescriptorHeap> m_dsvDescriptorHeap;
	// uint32_t					 m_dsvDescriptorSize = 0;

	//// 렌더링 리소스 추가 25.07.20
	// ComPtr<ID3D12RootSignature> m_rootSignature;
	// ComPtr<ID3D12PipelineState> m_pipelineState;
	// ComPtr<ID3D12Resource>		m_vertexBuffer;
	// D3D12_VERTEX_BUFFER_VIEW	m_vertexBufferView;

	//// 인덱스 버퍼 추가 25.07.20
	// ComPtr<ID3D12Resource>	m_indexBuffer;
	// D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

	//// Ground Constant Buffer
	// ComPtr<ID3D12Resource> m_constantBuffer2;
	// ConstantBuffer		   m_constantBufferData2;
	// UINT8*				   m_pCbvDataBegin2 = nullptr;

	// Ground 객체 추가
	//std::unique_ptr<Ground> m_ground;


private:
	HWND	  m_hWnd = nullptr;
	HINSTANCE m_hInstance = nullptr;
};