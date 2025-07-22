#pragma once
#include "stdafxClientFramework.h"
#include "DxSwapChain.h"
#include "DxCommandContextPool.h"

// MVP 행렬 추가(상수버퍼) 25.07.20
struct ConstantBuffer
{
	DirectX::XMFLOAT4X4 mvp;  // Model-View-Projection 행렬
	//XMat4x4
};

//// 인스턴싱을 위한 구조체
//struct InstanceData
//{
//	DirectX::XMFLOAT4X4 world; // 각 인스턴스의 월드 행렬
//	DirectX::XMFLOAT4 color;   // 각 인스턴스의 색상
//};
//
//struct PerFrameData
//{
//	DirectX::XMFLOAT4X4 view;       // 뷰 행렬
//	DirectX::XMFLOAT4X4 projection; // 프로젝션 행렬
//};

class GameFramework
{
public:
	GameFramework() = default;

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

private:
	// 새로 추가할 멤버들 25.07.19
	std::unique_ptr<DxSwapChain> m_swapChain;
	ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
	uint32_t m_rtvDescriptorSize = 0;
	std::unique_ptr<DxCommandContextPool> m_commandContextPool;

	// 렌더링 리소스 추가 25.07.20
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pipelineState;	
	ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

	// 인덱스 버퍼 추가 25.07.20
	ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

	// 상수 버퍼 추가 25.07.20
	ComPtr<ID3D12Resource> m_constantBuffer;
	ConstantBuffer m_constantBufferData;
	UINT8* m_pCbvDataBegin = nullptr;	//시작 주소

	//Ground Constant Buffer
	ComPtr<ID3D12Resource> m_constantBuffer2;
	ConstantBuffer m_constantBufferData2;
	UINT8* m_pCbvDataBegin2 = nullptr;

	// 표시등 큐브용 상수버퍼
	ComPtr<ID3D12Resource> m_constantBuffer3;
	ConstantBuffer m_constantBufferData3;
	UINT8* m_pCbvDataBegin3 = nullptr;

	// 플레이어 위치 및 이동
	float m_playerX = 0.0f;
	float m_playerY = 0.5f;  
	float m_playerZ = 0.0f;
	float m_playerSpeed = 5.0f;  // 이동 속도

	//마우스로 카메라 이동
	bool m_isMouseDragging = false;     
	float m_cameraYaw = 0.0f;          //좌우
	float m_cameraPitch = 0.0f;			//위아래
	float m_cameraDistance = 15.0f;    
	float m_mouseSensitivity = 0.005f; 
	float m_lastMouseX = 0.0f;
	float m_lastMouseY = 0.0f;

private:
	HWND m_hWnd = nullptr;
	HINSTANCE m_hInstance = nullptr;
};



