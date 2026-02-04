#pragma once
#include "UIComponent.h"
#include "DxMath.h"

// Texture Rendering Component
class ImageUIComponent : public UIComponent<ImageUIComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "ImageUIComponent"; }

	ImageUIComponent() = default;
	virtual ~ImageUIComponent() = default;

	// 기본 텍스처 설정
	void	 SetTexture(uint32_t textureId) { m_normalTextureId = textureId; }
	uint32_t GetTextureId() const { return m_normalTextureId; }

	// 9-Slice 테두리 설정
	void SetSliceBorder(float left, float top, float right, float bottom)
	{
		m_sliceBorder = {left, top, right, bottom};
	}

	DirectX::XMFLOAT4 GetSliceBorder() const { return m_sliceBorder; }

	// 상태 설정 (외부, 예를 들어 ButtonUI에서 호출)
	void SetState(ButtonState state) { m_currentState = state; }

	// 색상 설정
	void SetNormalColor(const DirectX::XMFLOAT4& color) { m_normalColor = color; }
	void SetHoverColor(const DirectX::XMFLOAT4& color) { m_hoverColor = color; }
	void SetPressedColor(const DirectX::XMFLOAT4& color) { m_pressedColor = color; }
	void SetDisabledColor(const DirectX::XMFLOAT4& color) { m_disabledColor = color; }

	// 텍스처 설정
	void SetNormalTexture(uint32_t textureId) { m_normalTextureId = textureId; }
	void SetHoverTexture(uint32_t textureId) { m_hoverTextureId = textureId; }
	void SetPressedTexture(uint32_t textureId) { m_pressedTextureId = textureId; }
	void SetDisabledTexture(uint32_t textureId) { m_disabledTextureId = textureId; }

	// RenderData 수집
	virtual void GetRenderData(std::vector<UIRenderData>& outData) override;

private:
	// 상태 관리
	ButtonState m_currentState = ButtonState::Normal;

	// 상태별 텍스처 ID
	uint32_t m_normalTextureId = 0;
	uint32_t m_hoverTextureId = 0;
	uint32_t m_pressedTextureId = 0;
	uint32_t m_disabledTextureId = 0;

	// 상태별 색상 (텍스처와 곱해지거나 텍스처가 없을 때 사용)
	DirectX::XMFLOAT4 m_normalColor = {1.0f, 1.0f, 1.0f, 1.0f};
	DirectX::XMFLOAT4 m_hoverColor = {1.0f, 1.0f, 1.0f, 1.0f};
	DirectX::XMFLOAT4 m_pressedColor = {1.0f, 1.0f, 1.0f, 1.0f};
	DirectX::XMFLOAT4 m_disabledColor = {1.0f, 1.0f, 1.0f, 1.0f};

	// 9-Slice 테두리 정보
	// 0이면 일반 이미지 모드
	DirectX::XMFLOAT4 m_sliceBorder = {0.0f, 0.0f, 0.0f, 0.0f};
};