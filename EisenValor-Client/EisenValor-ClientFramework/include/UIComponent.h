#pragma once
#include "DxCommon.h"
#include "DxMath.h"  
class UIComponent
{

protected:
	bool m_isSelected = false;

public:
	virtual ~UIComponent() = default;
	virtual void Initialize() = 0;
	virtual void Render(ID3D12GraphicsCommandList* cmdList) = 0;

	void SetPosition(float x, float y) { m_position = {x, y}; }
	void SetSize(float width, float height) { m_size = {width, height}; }
	void SetColor(const DirectX::XMFLOAT4& color) { m_color = color; }
	void SetTexture(uint32_t textureId) { m_textureId = textureId; }

	DirectX::XMFLOAT2 GetPosition() const { return m_position; }
	DirectX::XMFLOAT2 GetSize() const { return m_size; }
	virtual DirectX::XMFLOAT4 GetColor() const { return m_color; }
	uint32_t	GetTextureId() const { return m_textureId; }

	virtual const char* GetUIType() const = 0;
	void				SetSelected(bool selected) { m_isSelected = selected; }
	bool				IsSelected() const { return m_isSelected; }

protected:
	DirectX::XMFLOAT2 m_position = {0.0f, 0.0f};
	DirectX::XMFLOAT2 m_size = {100.0f, 100.0f};
	DirectX::XMFLOAT4 m_color = {1.0f, 0.0f, 0.0f, 1.0f}; // 빨간색 기본값
	uint32_t m_textureId = 0;
};