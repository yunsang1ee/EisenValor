#include "stdafxClientFramework.h"
#include "ImageUIComponent.h"
#include "RectTransformComponent.h"
#include "UIGlobal.h"
#include "TextureResource.h"

void ImageUIComponent::GetRenderData(std::vector<UIRenderData>& outData)
{
	// GameObject 활성화 상태 체크
	if (auto* go = GetGameObject())
	{
		if (!go->IsActive())
		{
			return;
		}
	}

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
	std::shared_ptr<TextureResource> targetTextureResource = nullptr;
	DirectX::XMFLOAT4 targetStateColor = {1.f, 1.f, 1.f, 1.f};

	switch (m_currentState)
	{
	case ButtonState::Normal:
		targetTextureResource = m_normalTextureResource;
		targetStateColor = m_normalColor;
		break;
	case ButtonState::Hover:
		targetTextureResource = m_hoverTextureResource;
		targetStateColor = m_hoverColor;
		break;
	case ButtonState::Pressed:
		targetTextureResource = m_pressedTextureResource;
		targetStateColor = m_pressedColor;
		break;
	case ButtonState::Disabled:
		targetTextureResource = m_disabledTextureResource;
		targetStateColor = m_disabledColor;
		break;
	}

	// 상태별 텍스처 리소스가 지정되지 않은 경우 Normal 리소스로 대체
	if (targetTextureResource == nullptr)
	{
		targetTextureResource = m_normalTextureResource;
	}

	// 리소스가 없거나 GPU 로딩이 완료되지 않았으면
	if (targetTextureResource == nullptr || !targetTextureResource->IsReady())
	{
		return;
	}

	// UIRenderData 생성
	UIRenderData data;
	data.textureResource = targetTextureResource;
	data.rect = finalRect;
	data.uvMin = {0.0f, 0.0f};
	data.uvMax = {1.0f, 1.0f};
	data.rotationDegrees = rectTr->GetRotationDegrees();

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
