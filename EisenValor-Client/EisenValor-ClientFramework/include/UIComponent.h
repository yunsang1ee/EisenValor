#pragma once
#include "IComponent.h"
#include "RectTransformComponent.h"
#include "DxMath.h"
#include <vector>
#include "GameObject.h"

// UI 상호작용 상태 (UIComponent들이 공유)
enum class ButtonState
{
	Normal,
	Hover,
	Pressed,
	Disabled
};

// UIRenderPass가 RenderData를 수집하여 Batching
struct UIRenderData
{
	uint32_t			textureId; // 텍스처 핸들
	RectTransformComponent::Rect rect;	   // RectTransform가 계산
	DirectX::XMFLOAT4	color;
	DirectX::XMFLOAT2	uvMin;
	DirectX::XMFLOAT2	uvMax;
};

class IUIComponent
{
public:
	virtual ~IUIComponent() = default;
	// Z-Order
	virtual int32_t GetOrder() const = 0;
	// 렌더링 데이터 수집
	virtual void GetRenderData(std::vector<UIRenderData>& outData) = 0;
};

// 모든 UI 요소의 기반인 추상 클래스
template <typename T>
class UIComponent : public ComponentBase<T>, public IUIComponent
{
public:
	static constexpr const char* GetStaticTypeName() { return "UIComponent"; }

	virtual ~UIComponent() = default;

	// IComponent 생명주기
	virtual void OnAttach() override {}
	virtual void OnDetach() override {}

	// 같은 GameObject에 붙어있는 RectTransform 반환
	// 위치, 크기, 앵커 정보는 RectTransform에
	RectTransformComponent* GetRectTransform() const { return this->GetGameObject()->GetComponent<RectTransformComponent>(); }

	// Z-Order
	virtual int32_t GetOrder() const { return m_order; }
	void	SetOrder(int32_t order) { m_order = order; }

	// 인터랙션
	bool IsInteractable() const { return m_isInteractable; }
	void SetInteractable(bool interactable) { m_isInteractable = interactable; }

	// 색상
	DirectX::XMFLOAT4 GetColor() const { return m_color; }
	void			  SetColor(const DirectX::XMFLOAT4& color) { m_color = color; }

	// UI 렌더링 데이터 수집
	virtual void GetRenderData(std::vector<UIRenderData>& outData) override = 0;

protected:
	DirectX::XMFLOAT4 m_color = {1.0f, 1.0f, 1.0f, 1.0f}; // 하얀색
	int32_t			  m_order = 0;						  // 렌더링 우선순위
	bool			  m_isInteractable = true;			  // 상호작용 활성화 여부
};
