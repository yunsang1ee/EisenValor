#include "stdafxClient.h"
#include "ScoreScene.h"

#include "ImageUIComponent.h"
#include "RectTransformComponent.h"
#include "ResourceGlobal.h"
#include "TextUIComponent.h"
#include "TextureResource.h"

uint8 ScoreScene::s_redScore = 0;
uint8 ScoreScene::s_blueScore = 0;

void ScoreScene::SetScores(uint8 redScore, uint8 blueScore)
{
	s_redScore = redScore;
	s_blueScore = blueScore;
}

void ScoreScene::OnRegisterCustomComponents() {}

void ScoreScene::OnStartImpl()
{
	ReserveGameObject(
		"ScoreSceneBackground", std::nullopt,
		[this](GameObject* obj)
		{
			CreateComponentWithInit<RectTransformComponent>(
				obj->GetHandle(),
				[](RectTransformComponent* rect)
				{
					rect->SetAnchors({0.0f, 0.0f}, {1.0f, 1.0f});
					rect->SetPivot({0.5f, 0.5f});
					rect->SetOffsetMin({0.0f, 0.0f});
					rect->SetOffsetMax({0.0f, 0.0f});
				}
			);

			CreateComponentWithInit<ImageUIComponent>(
				obj->GetHandle(),
				[](ImageUIComponent* image)
				{
					auto texture = GLOBAL(ResourceGlobal).Load<TextureResource>(
						L"Resource\\Texture\\Scene\\scorescene.evtex");
					image->SetNormalTextureResource(texture);
					image->SetOrder(0);
				}
			);
		}
	);

	const auto createScoreText = [this](const char* name, float anchorX, uint8 score)
	{
		ReserveGameObject(
			name, std::nullopt,
			[this, anchorX, score](GameObject* obj)
			{
				CreateComponentWithInit<RectTransformComponent>(
					obj->GetHandle(),
					[anchorX](RectTransformComponent* rect)
					{
						rect->SetAnchors({anchorX, 0.5f}, {anchorX, 0.5f});
						rect->SetPivot({0.5f, 0.5f});
						rect->SetOffsetMin({-160.0f, -80.0f});
						rect->SetOffsetMax({160.0f, 80.0f});
					}
				);

				CreateComponentWithInit<TextUIComponent>(
					obj->GetHandle(),
					[score](TextUIComponent* text)
					{
						text->SetText(std::to_wstring(score));
						text->SetFontSize(72.0f);
						text->SetHorizontalAlign(TextHorizontalAlign::Center);
						text->SetVerticalAlign(TextVerticalAlign::Center);
						text->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
						text->SetOrder(1);
					}
				);
			}
		);
	};

	createScoreText("RedScoreText", 0.25f, s_redScore);
	createScoreText("BlueScoreText", 0.75f, s_blueScore);
}

void ScoreScene::OnEndImpl() {}
