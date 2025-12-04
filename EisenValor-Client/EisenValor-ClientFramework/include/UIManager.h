// UIManager.h
#pragma once
#include "stdafxClientFramework.h"
#include "../../EisenValor-Server/ServerEngine/Singleton.hpp"
#include "UIComponent.h"

class UIManager : public Singleton<UIManager>
{
public:
	UIManager() = default;
	virtual ~UIManager() = default;
	friend class Singleton;

	// 초기화
	void Initialize(ID3D12Device* device);
	void Shutdown();

	// UI 전용 파이프라인 상태 관리
	ID3D12PipelineState* GetUIPipelineState() const { return m_uiPipelineState.Get(); }

	// 전역 UI 관리 (미니맵, 인벤토리, 메뉴 등)
	template <typename T, typename... Args>
	T* AddGlobalUI(Args&&... args)
	{
		auto ui = std::make_unique<T>(std::forward<Args>(args)...);
		T*	 ptr = ui.get();
		ui->Initialize(m_device);
		m_globalUIComponents.push_back(std::move(ui));
		return ptr;
	}

	void RemoveGlobalUI(UIComponent* ui);

	// 렌더링
	void RenderAllUI(ID3D12GraphicsCommandList* cmdList, DirectX::XMMATRIX view, DirectX::XMMATRIX projection);
	void UpdateAllUI(float deltaTime);

	// 디버그
	void PrintUIStats() const;

private:
	void CreateUIPipelineState();

private:
	ID3D12Device*				m_device = nullptr;
	ComPtr<ID3D12PipelineState> m_uiPipelineState;

	// 전역 UI 컴포넌트들 (화면에 고정된 UI들)
	std::vector<std::unique_ptr<UIComponent>> m_globalUIComponents;
};

// UIManager.cpp
#include "stdafxClientFramework.h"
#include "UIManager.h"
#include "GameObjectManager.h"
#include "GlobalInterfaces.h"

void UIManager::Initialize(ID3D12Device* device)
{
	m_device = device;
	CreateUIPipelineState();
}

void UIManager::CreateUIPipelineState()
{
	// 기존 파이프라인 상태를 복사해서 UI용으로 수정
	// GameFramework의 파이프라인 생성 코드를 참조해야 함

	// TODO: 셰이더 컴파일 및 파이프라인 상태 생성
	// 깊이 테스트 OFF, 블렌딩 ON 등 UI에 적합한 설정
}

void UIManager::RenderAllUI(ID3D12GraphicsCommandList* cmdList, DirectX::XMMATRIX view, DirectX::XMMATRIX projection)
{
	// UI 전용 파이프라인 상태 설정
	cmdList->SetPipelineState(m_uiPipelineState.Get());

	// 모든 GameObject의 UI 렌더링
	MANAGER(GameObjectManager)->RenderUI(cmdList, view, projection);

	// 전역 UI 렌더링
	for (auto& ui : m_globalUIComponents)
	{
		if (ui && ui->IsActive())
		{
			ui->Render(cmdList, view, projection);
		}
	}
}

void UIManager::UpdateAllUI(float deltaTime)
{
	// 전역 UI 업데이트
	for (auto& ui : m_globalUIComponents)
	{
		if (ui && ui->IsActive())
		{
			ui->Update(deltaTime);
		}
	}
}

void UIManager::PrintUIStats() const
{
	std::cout << "UIManager Stats:" << std::endl;
	std::cout << "  Global UI Components: " << m_globalUIComponents.size() << std::endl;

	for (const auto& ui : m_globalUIComponents)
	{
		std::cout << "    - " << ui->GetUIType() << " (Active: " << ui->IsActive() << ")" << std::endl;
	}
}
#pragma once
