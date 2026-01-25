#include "stdafxClientFramework.h"
#include "ImageUIComponent.h"
#include "RectTransformComponent.h"
#include "UIGlobal.h"

void ImageUIComponent::GetRenderData(std::vector<UIRenderData>& outData)
{
	// RectTransformComponent 가져오기
	RectTransformComponent* rectTr = GetRectTransform();
	if (!rectTr)
	{
		return;
	}

	// Rect 계산
	RectTransformComponent::Rect finalRect = rectTr->GetRect();

	//// 디버깅용
	// static bool s_hasLoggedRect = false;
	// if (!s_hasLoggedRect)
	//{
	//	DEBUG_LOG_FMT("ImageUI GetRenderData - Rect: x={}, y={}, w={}, h={}\n", finalRect.x, finalRect.y,
	// finalRect.width, finalRect.height); 	s_hasLoggedRect = true;
	// }

	// 9-Slice 사용 여부
	bool use9Slice =
		(m_sliceBorder.x > 0.0f || m_sliceBorder.y > 0.0f || m_sliceBorder.z > 0.0f || m_sliceBorder.w > 0.0f);

	// 상태에 따라 렌더링할 텍스처와 색상 결정
	uint32_t		  targetTextureId = 0;
	DirectX::XMFLOAT4 targetStateColor = {1.f, 1.f, 1.f, 1.f};

	switch (m_currentState)
	{
	case ButtonState::Normal:
		targetTextureId = m_normalTextureId;
		targetStateColor = m_normalColor;
		break;
	case ButtonState::Hover:
		targetTextureId = m_hoverTextureId;
		targetStateColor = m_hoverColor;
		break;
	case ButtonState::Pressed:
		targetTextureId = m_pressedTextureId;
		targetStateColor = m_pressedColor;
		break;
	case ButtonState::Disabled:
		targetTextureId = m_disabledTextureId;
		targetStateColor = m_disabledColor;
		break;
	}

	// UIRenderData 생성
	UIRenderData data;
	data.textureId = targetTextureId;
	data.rect = finalRect;
	data.uvMin = {0.0f, 0.0f};
	data.uvMax = {1.0f, 1.0f};

	DirectX::XMFLOAT4 finalColor;
	finalColor.x = m_color.x * targetStateColor.x;
	finalColor.y = m_color.y * targetStateColor.y;
	finalColor.z = m_color.z * targetStateColor.z;
	finalColor.w = m_color.w * targetStateColor.w;
	data.color = finalColor;

	if (use9Slice)
	{
		// TODO : 9-Slice 렌더링 데이터 생성
		// 현재는 생략
		outData.push_back(data);
	}
	else
	{
		// 단일 쿼드 생성
		outData.push_back(data);
	}
}