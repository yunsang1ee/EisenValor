#pragma once
#include <UIComponent.h>

class BattleUIComponent : public UIComponent
{
public:
	void		Initialize() override;
	void		Render(ID3D12GraphicsCommandList* cmdList) override;
	const char* GetUIType() const override { return "BattleUI"; }

	DirectX::XMFLOAT4 GetColor() const override;
};