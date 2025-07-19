#pragma once
#include "stdafxClientFramework.h"
#include "DxSwapChain.h"
#include "DxCommandContextPool.h"

class GameFramework
{
public:
	GameFramework() = default;

	bool Initialize(HINSTANCE hInstance, HWND hwnd);
	void Run();
	void Release();

	HWND GetHWND() const noexcept { return m_hWnd; }

	LRESULT OnWindowMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	 
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

private:
	HWND m_hWnd = nullptr;
	HINSTANCE m_hInstance = nullptr;
};