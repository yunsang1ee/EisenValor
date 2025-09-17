#pragma once
#include "stdafxClientFramework.h"
#include "DxSwapChain.h"
#include "DxCommandContextPool.h"
#include "DxCommon.h"
#include "DxFeatureCaps.h"
#include "GameObject.h"
#include "Ground.h"
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

class Player;

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

	void InitializeRaytracing();
	void CreateRaytracingOutputTexture();
	void CreateRaytracingRootSignatures();
	void CreateRaytracingPipelineStateObject();
	void CreateShaderTable();
	void BuildAccelerationStructures();
	void BuildTopLevelAS(
		ID3D12GraphicsCommandList4* commandList, const std::vector<D3D12_RAYTRACING_INSTANCE_DESC>& instances
	);
	void RenderRaytracing();

	ComPtr<IDxcBlob> CompileShader(
		IDxcUtils*			dxcUtils,
		IDxcCompiler3*		dxcCompiler,
		IDxcIncludeHandler* includeHandler,
		LPCWSTR				fileName,
		LPCWSTR				entryPoint,
		LPCWSTR				target
	);

private:
	DxFeatureCaps m_featureCaps;
	// 새로 추가할 멤버들 25.07.19
	std::unique_ptr<DxSwapChain>		  m_swapChain;
	ComPtr<ID3D12DescriptorHeap>		  m_rtvDescriptorHeap;
	uint32_t							  m_rtvDescriptorSize = 0;
	std::unique_ptr<DxCommandContextPool> m_commandContextPool;

	// 렌더링 리소스 추가 25.07.20
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pipelineState;
	ComPtr<ID3D12Resource>		m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW	m_vertexBufferView;

	// 인덱스 버퍼 추가 25.07.20
	ComPtr<ID3D12Resource>	m_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

	// Ground Constant Buffer
	ComPtr<ID3D12Resource> m_constantBuffer2;
	ConstantBuffer		   m_constantBufferData2;
	UINT8*				   m_pCbvDataBegin2 = nullptr;


	ComPtr<ID3D12Device5>			   m_dxrDevice;

	ComPtr<ID3D12StateObject>			m_rtStateObject;
	ComPtr<ID3D12StateObjectProperties> m_rtStateObjectProps;

	ComPtr<ID3D12RootSignature> m_rtGlobalRootSignature;
	ComPtr<ID3D12RootSignature> m_rtLocalRootSignature;

	ComPtr<ID3D12Resource> m_topLevelAS;
	ComPtr<ID3D12Resource> m_bottomLevelAS;
	ComPtr<ID3D12Resource> m_tlasInstancesBuffer;

	ComPtr<ID3D12Resource> m_shaderTable;
	struct ShaderTableLayout
	{
		uint32_t rayGenOffset = 0;
		uint32_t rayGenSize = 0;
		uint32_t missOffset = 0;
		uint32_t missSize = 0;
		uint32_t hitGroupOffset = 0;
		uint32_t hitGroupSize = 0;
	} m_shaderTableLayout;

	ComPtr<ID3D12Resource>		 m_raytracingOutput;
	ComPtr<ID3D12DescriptorHeap> m_raytracingDescriptorHeap;

	// Ground 객체 추가
	std::unique_ptr<Ground> m_ground;

private:
	HWND	  m_hWnd = nullptr;
	HINSTANCE m_hInstance = nullptr;
};