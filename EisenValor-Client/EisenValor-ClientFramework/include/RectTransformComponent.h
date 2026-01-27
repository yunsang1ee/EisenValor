#pragma once
#include "stdafxClientFramework.h"
#include "IComponent.h"

// 부모 Rect 대비 상대적/절대적 위치와 크기 계산
class Transform;

class RectTransformComponent : public ComponentBase<RectTransformComponent>
{
public:
	static constexpr const char* GetStaticTypeName() 
	{ 
		return "RectTransform"; 
	}
	static constexpr int		 kPriority = -99;

	struct Rect
	{
		float x, y, width, height;

		bool Contains(const Vec2& MousePos) const
		{
			return MousePos.x >= x && MousePos.x <= x + width && MousePos.y >= y && MousePos.y <= y + height;
		}
	};

	RectTransformComponent();
	virtual ~RectTransformComponent() = default;

	// IComponent Lifecycle
	virtual void OnAttach() override { MarkDirty(); }

	// Layout 인터페이스
	Rect GetRect(); // Dirty 상태일 경우 재계산 후 반환
	bool Contains(Vec2 mousePos);

	// 설정 메서드
	void SetAnchors(Vec2 min, Vec2 max);
	void SetPivot(Vec2 pivot);
	void SetOffsetMin(Vec2 offsetMin);
	void SetOffsetMax(Vec2 offsetMax);

	// 편의 메서드: 앵커 사이의 간격을 유지하며 크기 설정
	void SetSizeDelta(Vec2 sizeDelta);

	// Getters
	Transform* GetTransform() const;
	Vec2 GetAnchorMin() const { return m_anchorMin; }
	Vec2 GetAnchorMax() const { return m_anchorMax; }
	Vec2 GetPivot() const { return m_pivot; }
	Vec2 GetOffsetMin() const { return m_offsetMin; }
	Vec2 GetOffsetMax() const { return m_offsetMax; }

	// 레이아웃 갱신 로직
	void UpdateLayout();
	void MarkDirty();

private:
	Rect GetParentRect();
	void UpdateChildrenLayouts();

	// 데이터 멤버
	Vec2 m_anchorMin = {0.5f, 0.5f}; // 부모 Rect 내 최소 앵커 비율 (0~1)
	Vec2 m_anchorMax = {0.5f, 0.5f}; // 부모 Rect 내 최대 앵커 비율 (0~1)
	Vec2 m_pivot = {0.5f, 0.5f};	 // 본인 Rect 내 기준점 비율 (0~1)
	Vec2 m_offsetMin = {0.0f, 0.0f}; // 앵커 최소점으로부터의 픽셀 오프셋
	Vec2 m_offsetMax = {0.0f, 0.0f}; // 앵커 최대점으로부터의 픽셀 오프셋

	Rect m_rect = {0.0f, 0.0f, 0.0f, 0.0f}; // 계산된 최종 화면 좌표 및 크기
	bool m_isDirty = true;
};
