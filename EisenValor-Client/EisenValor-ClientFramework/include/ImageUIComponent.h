#pragma once
#include "UIComponent.h"
#include "DxMath.h"

// Texture Rendering Component
class ImageUIComponent : public UIComponent<ImageUIComponent>
{
public:
	static constexpr const char* GetStaticTypeName() 
	{ 
		return "ImageUIComponent"; 
	}

	ImageUIComponent() = default;
	virtual ~ImageUIComponent() = default;

	void	 SetTexture(uint32_t textureId) { m_textureId = textureId; }
	uint32_t GetTextureId() const { return m_textureId; }

	// 9-Slice 테두리 설정
	void SetSliceBorder(float left, float top, float right, float bottom)
	{
		m_sliceBorder = {left, top, right, bottom};
	}

	DirectX::XMFLOAT4 GetSliceBorder() const { return m_sliceBorder; }

	// RenderPass에게 보낼 RenderData
	virtual void GetRenderData(std::vector<UIRenderData>& outData) override;

private:
	uint32_t m_textureId = 0;

	// 9-Slice 테두리 정보
	// 0이면 일반 이미지 모드
	DirectX::XMFLOAT4 m_sliceBorder = {0.0f, 0.0f, 0.0f, 0.0f};
};
