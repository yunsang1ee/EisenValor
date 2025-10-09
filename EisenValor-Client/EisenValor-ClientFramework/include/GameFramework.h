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

	void RecreateDepthStencilBuffer(uint32_t width, uint32_t height);

private:
	DxFeatureCaps m_featureCaps;
	// 새로 추가할 멤버들 25.07.19
	std::unique_ptr<DxSwapChain>		  m_swapChain;
	ComPtr<ID3D12DescriptorHeap>		  m_rtvDescriptorHeap;
	uint32_t							  m_rtvDescriptorSize = 0;
	std::unique_ptr<DxCommandContextPool> m_commandContextPool;

	// 깊이 버퍼 관련 멤버 추가
	ComPtr<ID3D12Resource>		 m_depthStencilBuffer;
	ComPtr<ID3D12DescriptorHeap> m_dsvDescriptorHeap;
	uint32_t					 m_dsvDescriptorSize = 0;

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


	// Ground 객체 추가
	std::unique_ptr<Ground> m_ground;

private:
	HWND	  m_hWnd = nullptr;
	HINSTANCE m_hInstance = nullptr;
};