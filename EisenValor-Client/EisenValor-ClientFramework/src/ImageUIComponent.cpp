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
	//static bool s_hasLoggedRect = false;
	//if (!s_hasLoggedRect)
	//{
	//	DEBUG_LOG_FMT("ImageUI GetRenderData - Rect: x={}, y={}, w={}, h={}\n", finalRect.x, finalRect.y, finalRect.width, finalRect.height);
	//	s_hasLoggedRect = true;
	//}

	// 9-Slice 사용 여부 
	bool use9Slice =
		(m_sliceBorder.x > 0.0f || m_sliceBorder.y > 0.0f || m_sliceBorder.z > 0.0f || m_sliceBorder.w > 0.0f);

	// UIRenderData 생성
	UIRenderData data;
	data.textureId = m_textureId;
	data.rect = finalRect;
	data.uvMin = {0.0f, 0.0f};
	data.uvMax = {1.0f, 1.0f};

	if (m_textureId == 0)
	{
		data.color = {1.0f, 0.0f, 0.0f, 1.0f}; // 빨간색
	}
	else
	{
		data.color = m_color; // 원래 색상
	}

	if (use9Slice)
	{
		// TODO : 9-Slice 렌더링 데이터 생성
		// 현재는 생략
		// 실제 구현 : 9개의 쿼드 데이터를 생성하여 outData.push_back()
		outData.push_back(data);
	}
	else
	{
		// 단일 쿼드 생성
		outData.push_back(data);
	}
}
