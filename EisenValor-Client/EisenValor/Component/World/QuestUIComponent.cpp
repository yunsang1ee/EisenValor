#include "stdafxClient.h"
#include "QuestUIComponent.h"

#include "GameObject.h"
#include "ImageUIComponent.h"
#include "RectTransformComponent.h"
#include "Scene.h"
#include "TextUIComponent.h"

void QuestUIComponent::OnUpdate(float deltaTime)
{
	m_elapsed += deltaTime;

	const float progress = std::clamp(m_elapsed / 0.25f, 0.0f, 1.0f);
	const float eased = 1.0f - (1.0f - progress) * (1.0f - progress);

	if (auto* image = GetGameObject()->GetComponent<ImageUIComponent>())
	{
		image->SetColor({1.0f, 1.0f, 1.0f, eased});
	}

	if (auto* text = GetGameObject()->GetComponent<TextUIComponent>())
	{
		text->SetColor({1.0f, 1.0f, 1.0f, eased});
	}

	if (auto* rect = GetGameObject()->GetComponent<RectTransformComponent>())
	{
		const float y = 40.0f - 20.0f * eased;
		rect->SetOffsetMin({-320.0f, -80.0f + y});
		rect->SetOffsetMax({320.0f, 240.0f + y});
	}
}

void QuestUIComponent::SetMessage(const std::wstring& message)
{
	if (auto* text = GetGameObject()->GetComponent<TextUIComponent>())
	{
		text->SetText(message);
	}

	RestartIntroAnimation();
}

void QuestUIComponent::RestartIntroAnimation()
{
	m_elapsed = 0.0f;
}
