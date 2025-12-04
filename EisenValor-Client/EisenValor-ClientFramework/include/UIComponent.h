// UIComponent.h
#pragma once
#include "stdafxClientFramework.h"
#include "DxCommon.h"

class GameObject;

class UIComponent
{
public:
	UIComponent() = default;
	virtual ~UIComponent() = default;

	// 순수 가상 함수
	virtual void Initialize(ID3D12Device* device) = 0;
	virtual void Update(float deltaTime) {}
	virtual void Render(ID3D12GraphicsCommandList* cmdList, DirectX::XMMATRIX view, DirectX::XMMATRIX projection) = 0;

	// 소유자 설정
	virtual void SetOwner(GameObject* owner) { m_owner = owner; }
	GameObject*	 GetOwner() const { return m_owner; }

	// 활성화 상태
	void SetActive(bool active) { m_isActive = active; }
	bool IsActive() const { return m_isActive; }

	// UI 타입 (디버깅용)
	virtual const char* GetUIType() const = 0;

protected:
	GameObject* m_owner = nullptr;
	bool		m_isActive = true;
};