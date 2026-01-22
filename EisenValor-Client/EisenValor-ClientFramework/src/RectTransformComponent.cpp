#include "stdafxClientFramework.h"
#include "RectTransformComponent.h"
#include "GameObject.h"
#include "Transform.h"
#include "Scene.h"
#include "DxRendererGlobal.h"
#include "DxSwapChain.h"

RectTransformComponent::RectTransformComponent() {}

RectTransformComponent::Rect RectTransformComponent::GetRect()
{
	if (m_isDirty)
	{
		UpdateLayout();
	}
	return m_rect;
}

bool RectTransformComponent::Contains(Vec2 mousePos)
{
	if (m_isDirty)
	{
		UpdateLayout();
	}
	return m_rect.Contains(mousePos);
}

void RectTransformComponent::SetAnchors(Vec2 min, Vec2 max)
{
	m_anchorMin = min;
	m_anchorMax = max;
	MarkDirty();
}

void RectTransformComponent::SetPivot(Vec2 pivot)
{
	m_pivot = pivot;
	MarkDirty();
}

void RectTransformComponent::SetOffsetMin(Vec2 offsetMin)
{
	m_offsetMin = offsetMin;
	MarkDirty();
}
void RectTransformComponent::SetOffsetMax(Vec2 offsetMax)
{
	m_offsetMax = offsetMax;
	MarkDirty();
}

void RectTransformComponent::SetSizeDelta(Vec2 sizeDelta)
{
	// 현재 중심에서 대칭적으로 확장하거나 단순히 설정하는 간단한 버전
	// 현재 오프셋 확인
	Vec2 currentSizeDelta = m_offsetMax - m_offsetMin;
	Vec2 diff = sizeDelta - currentSizeDelta;

	m_offsetMin -= diff * m_pivot;
	m_offsetMax += diff * (Vec2(1.0f, 1.0f) - m_pivot);

	MarkDirty();
}

void RectTransformComponent::UpdateLayout()
{
	// 1. 부모를 기준으로 자신의 Rect 계산
	Rect parentRect = GetParentRect();


	float parentWidth = parentRect.width;
	float parentHeight = parentRect.height;
	float parentX = parentRect.x;
	float parentY = parentRect.y;

	// 앵커에 따른 기준점 계산 (피벗 미적용 상태)
	float anchorPosX = parentX + (parentWidth * m_anchorMin.x);
	float anchorPosY = parentY + (parentHeight * m_anchorMin.y);

	// 오프셋 적용
	float x = anchorPosX + m_offsetMin.x;
	float y = anchorPosY + m_offsetMin.y;
	float right = parentX + (parentWidth * m_anchorMax.x) + m_offsetMax.x;
	float bottom = parentY + (parentHeight * m_anchorMax.y) + m_offsetMax.y;

	m_rect.width = right - x;
	m_rect.height = bottom - y;

	// 최종 좌표 결정: x, y는 좌상단 좌표여야 함.
	// m_pivot은 anchorPos + offset 위치가 Rect의 어디에 해당하는지를 결정함.
	// (예: pivot 0.5면 해당 위치가 Rect의 중앙이 되도록 좌상단 x, y를 보정)
	m_rect.x = x; 
	m_rect.y = y;

	m_isDirty = false;

	// 2. 자식들을 순회하며 업데이트
	UpdateChildrenLayouts();
}

RectTransformComponent::Rect RectTransformComponent::GetParentRect()
{
	Rect parentRect = {0.0f, 0.0f, (float)kDefaultWindowWidth, (float)kDefaultWindowHeight};

	auto* swapChain = DxRendererGlobal::GetInstance().GetSwapChain();
	if (swapChain)
	{
		parentRect.width = (float)swapChain->GetWidth();
		parentRect.height = (float)swapChain->GetHeight();
	}

	GameObject* owner = GetGameObject();
	if (!owner)
	{
		return parentRect;
	}

	Transform&		  tr = owner->GetTransform();
	Transform::Handle parentHandle = tr.GetParent();
	if (!parentHandle.IsValid())
	{
		return parentRect;
	}

	Scene* scene = owner->GetScene();
	if (!scene)
	{
		return parentRect;
	}

	auto* trStorage = scene->GetStorage<Transform>();
	if (!trStorage)
	{
		return parentRect;
	}

	Transform* parentTr = trStorage->Get(parentHandle);
	if (!parentTr)
	{
		return parentRect;
	}

	GameObject* parentObj = parentTr->GetGameObject();
	if (!parentObj)
	{
		return parentRect;
	}

	// 부모가 RectTransform을 가지고 있는지 확인
	RectTransformComponent* parentRectTr = parentObj->GetComponent<RectTransformComponent>();
	if (!parentRectTr)
	{
		return parentRect;
	}

	return parentRectTr->GetRect();
}

void RectTransformComponent::UpdateChildrenLayouts()
{
	GameObject* owner = GetGameObject();
	if (!owner)
	{
		return;
	}

	Scene* scene = owner->GetScene();
	if (!scene)
	{
		return;
	}

	auto* trStorage = scene->GetStorage<Transform>();
	if (!trStorage)
	{
		return;
	}

	Transform&	tr = owner->GetTransform();
	const auto& children = tr.GetChildren();

	for (auto childHandle : children)
	{
		Transform* childTr = trStorage->Get(childHandle);
		if (!childTr)
		{
			continue;
		}

		GameObject* childObj = childTr->GetGameObject();
		if (!childObj)
		{
			continue;
		}

		RectTransformComponent* childRectTr = childObj->GetComponent<RectTransformComponent>();
		if (childRectTr)
		{
			childRectTr->UpdateLayout();
		}
	}
}