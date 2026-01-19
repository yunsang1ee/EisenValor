#pragma once
#include <UIComponent.h>

class TestUIComponent : public UIComponent
{
public:
	void		Initialize() override {}
	void		Render(ID3D12GraphicsCommandList* cmdList) override;
	const char* GetUIType() const override { return "TestUI"; }
};