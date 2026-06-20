#include "stdafxClientFramework.h"
#include "TextUIComponent.h"
#include "RectTransformComponent.h"
#include "TextureResource.h"

void TextUIComponent::GetRenderData(std::vector<UIRenderData>& outData)
{
	if (auto* go = GetGameObject())
	{
		if (!go->IsActive())
		{
			return;
		}
	}

	RectTransformComponent* rectTr = GetRectTransform();
	if (!rectTr)
	{
		return;
	}

	if (m_textTextureResource == nullptr || !m_textTextureResource->IsReady())
	{
		return;
	}

	UIRenderData data;
	data.textureResource = m_textTextureResource;
	data.rect = rectTr->GetRect();
	data.uvMin = {0.0f, 0.0f};
	data.uvMax = {1.0f, 1.0f};
	data.rotationDegrees = rectTr->GetRotationDegrees();
	data.color = m_color;

	outData.push_back(data);
}
