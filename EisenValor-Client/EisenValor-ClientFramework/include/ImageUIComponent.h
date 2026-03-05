#pragma once
#include "UIComponent.h"
#include "DxMath.h"
#include <memory>

class TextureResource;

// Texture Rendering Component
class ImageUIComponent : public UIComponent<ImageUIComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "ImageUIComponent"; }

	ImageUIComponent() = default;
	virtual ~ImageUIComponent() = default;

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

	// 텍스처 리소스 설정
	void SetNormalTextureResource(std::shared_ptr<TextureResource> res) { m_normalTextureResource = res; }
	void SetHoverTextureResource(std::shared_ptr<TextureResource> res) { m_hoverTextureResource = res; }
	void SetPressedTextureResource(std::shared_ptr<TextureResource> res) { m_pressedTextureResource = res; }
	void SetDisabledTextureResource(std::shared_ptr<TextureResource> res) { m_disabledTextureResource = res; }

	// RenderData 수집
	virtual void GetRenderData(std::vector<UIRenderData>& outData) override;

private:
	// 상태 관리
	ButtonState m_currentState = ButtonState::Normal;

	// 상태별 텍스처 리소스
	std::shared_ptr<TextureResource> m_normalTextureResource;
	std::shared_ptr<TextureResource> m_hoverTextureResource;
	std::shared_ptr<TextureResource> m_pressedTextureResource;
	std::shared_ptr<TextureResource> m_disabledTextureResource;

	// 상태별 색상 (텍스처와 곱해지거나 텍스처가 없을 때 사용)
	DirectX::XMFLOAT4 m_normalColor = {1.0f, 1.0f, 1.0f, 1.0f};
	DirectX::XMFLOAT4 m_hoverColor = {1.0f, 1.0f, 1.0f, 1.0f};
	DirectX::XMFLOAT4 m_pressedColor = {1.0f, 1.0f, 1.0f, 1.0f};
	DirectX::XMFLOAT4 m_disabledColor = {1.0f, 1.0f, 1.0f, 1.0f};

	// 9-Slice 테두리 정보
	// 0이면 일반 이미지 모드
	DirectX::XMFLOAT4 m_sliceBorder = {0.0f, 0.0f, 0.0f, 0.0f};
};