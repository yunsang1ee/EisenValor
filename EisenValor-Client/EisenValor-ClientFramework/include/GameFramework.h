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

//// 인스턴싱을 위한 구조체
// struct InstanceData
//{
//	DX::XMFLOAT4X4 world; // 각 인스턴스의 월드 행렬
//	DX::XMFLOAT4 color;   // 각 인스턴스의 색상
// };
//
// struct PerFrameData
//{
//	DX::XMFLOAT4X4 view;       // 뷰 행렬
//	DX::XMFLOAT4X4 projection; // 프로젝션 행렬
// };

class Actor;

struct alignas(16) VertexPNU
{
	DX::XMFLOAT3 position;
	float		 pad_0;
	DX::XMFLOAT3 normal;
	float		 pad_1;
	DX::XMFLOAT2 uv;
	DX::XMFLOAT2 pad_2;
	VertexPNU(DX::XMFLOAT3 pos, DX::XMFLOAT3 nrm, DX::XMFLOAT2 tex)
		: position(pos), pad_0(0.0f), normal(nrm), pad_1(0.0f), uv(tex), pad_2{0.0f, 0.0f}
	{
	}
};
static_assert(sizeof(VertexPNU) % 16 == 0, "VertexPNU size must be multiple of 16 bytes");

struct alignas(16) PBRMaterial
{
	DX::XMFLOAT3 albedo;
	float		 metallic;
	float		 roughness;
	DX::XMFLOAT3 pad_0;
	DX::XMFLOAT3 emissive;
	float		 emissiveStrength;
};
static_assert(sizeof(PBRMaterial) % 16 == 0, "PBRMaterial size must be multiple of 16 bytes");

struct GeoInfo
{
	uint32_t vertexBase;
	uint32_t indexBase;
	uint32_t vertexCount;
	uint32_t indexCount;
};
static_assert(sizeof(GeoInfo) % 16 == 0, "GeoInfo size must be multiple of 16 bytes");

class GameFramework
{
public:
	GameFramework() = default;
	~GameFramework() { Release(); }

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

	std::unique_ptr<DxRtPipelineState>	m_rtPipeline;
	std::unique_ptr<DxRtPipelineState>	m_ptPipeline;
	std::unique_ptr<DxRtShaderTable>	m_rtShaderTable;
	std::unique_ptr<DxRtShaderTable>	m_ptShaderTable;
	bool								m_usePathTracing = false;

	std::unique_ptr<DxTLAS>				m_tlas;
	std::vector<std::unique_ptr<Actor>> m_sceneActors;

	DxTexture m_raytracingOutput;
	DxBuffer  m_materialBuffer;
	DxBuffer  m_vertexBuffer;
	DxBuffer  m_indexBuffer;
	DxBuffer  m_geoInfoBuffer;
	DxBuffer  m_instGeoBaseBuffer;

	DxDescriptorRange m_bufferRange;

	std::vector<VertexPNU>	 m_allVertices;
	std::vector<uint32_t>	 m_allIndices;
	std::vector<GeoInfo>	 m_geoInfoTable;
	std::vector<uint32_t>	 m_instGeoBase;
	std::vector<PBRMaterial> m_materials;

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
	// std::unique_ptr<Ground> m_ground;


private:
	HWND	  m_hWnd = nullptr;
	HINSTANCE m_hInstance = nullptr;
};