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

void RectTransformComponent::MarkDirty()
{
	m_isDirty = true;

	GameObject* owner = GetGameObject();
	if (!owner)
		return;

	Scene* scene = owner->GetScene();
	if (!scene)
		return;

	auto* trStorage = scene->GetStorage<Transform>();
	if (!trStorage)
		return;

	Transform& tr = owner->GetTransform();
	const auto& children = tr.GetChildren();

	for (auto childHandle : children)
	{
		Transform* childTr = trStorage->Get(childHandle);
		if (!childTr)
			continue;

		GameObject* childObj = childTr->GetGameObject();
		if (!childObj)
			continue;

		if (RectTransformComponent* childRectTr = childObj->GetComponent<RectTransformComponent>())
		{
			childRectTr->MarkDirty();
		}
	}
}

void RectTransformComponent::UpdateLayout()
{
	// 0. 해상도 동기화
	Vec2 scaleFactor = { 1.0f, 1.0f };
	auto* swapChain = DxRendererGlobal::GetInstance().GetSwapChain();
	if (swapChain && kDefaultWindowWidth > 0 && kDefaultWindowHeight > 0)
	{
		scaleFactor.x = (float)swapChain->GetWidth() / (float)kDefaultWindowWidth;
		scaleFactor.y = (float)swapChain->GetHeight() / (float)kDefaultWindowHeight;
	}

	// 1. 부모 Rect 정보 획득
	Rect parentRect = GetParentRect();
	Vec2 parentPos = { parentRect.x, parentRect.y };
	Vec2 parentSize = { parentRect.width, parentRect.height };

	// 2. 앵커 및 기본 영역 계산
	Vec2 anchorMinPos = parentPos + (parentSize * m_anchorMin);
	Vec2 anchorMaxPos = parentPos + (parentSize * m_anchorMax);

	// 스케일이 적용된 오프셋 계산
	Vec2 scaledOffsetMin = m_offsetMin * scaleFactor;
	Vec2 scaledOffsetMax = m_offsetMax * scaleFactor;

	// 3. 크기 결정
	// 최종 영역의 크기 (스케일된 오프셋 사용)
	Vec2 size = (anchorMaxPos + scaledOffsetMax) - (anchorMinPos + scaledOffsetMin);
	m_rect.width = size.x;
	m_rect.height = size.y;

	// 4. 앵커 기준점, 오프셋
	// anchorPos: 앵커 사각형 내부에서 피봇 어딘지
	Vec2 anchorPos = anchorMinPos + (anchorMaxPos - anchorMinPos) * m_pivot;

	// anchoredOffset: 앵커 기준점에서 실제 Pivot까지 거리
	Vec2 anchoredOffset = scaledOffsetMin + (scaledOffsetMax - scaledOffsetMin) * m_pivot;

	// 5. 최종 좌표 (UI의 좌상단 좌표)
	// (Pivot의 절대 위치) - (Pivot에서 좌상단까지의 거리)
	Vec2 finalPos = anchorPos + anchoredOffset - (size * m_pivot);
	m_rect.x = finalPos.x;
	m_rect.y = finalPos.y;

	m_isDirty = false;

	// 6. 자식 레이아웃 갱신
	UpdateChildrenLayouts();
}

Transform& RectTransformComponent::GetTransform() const
{
	return GetGameObject()->GetTransform();
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

	Rect result = parentRectTr->GetRect();

	return result;
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