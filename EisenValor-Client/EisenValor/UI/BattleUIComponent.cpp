#include "stdafxClient.h"
#include "BattleUIComponent.h"

void BattleUIComponent::Initialize()
{
}

void BattleUIComponent::Render(ID3D12GraphicsCommandList* cmdList)
{
	//RenderPass에서 렌더링
}

DirectX::XMFLOAT4 BattleUIComponent::GetColor() const
{
	if (m_isSelected)
	{
		return {1.0f, 1.0f, 1.0f, 1.0f}; // 선택 시 하얀색
	}
	return m_color; // 기본 빨간색
}