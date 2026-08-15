#include "stdafxClient.h"
#include "WorldSceneControllerComponent.h"
#include "GameObject.h"
#include "InputGlobal.h"
#include "ImageUIComponent.h"
#include "NetworkGlobal.h"
#include "RectTransformComponent.h"
#include "ResourceGlobal.h"
#include "Scene.h"
#include "SceneGlobal.h"
#include "TextureResource.h"
#include "Util/GameConstants.h"
#include "Packets/C2SPackets.h"
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
#include "RenderPass/RestirDebugGlobal.h"
#endif

namespace
{
std::wstring FormatBlueScore(uint8_t score)
{
	return L"BLUE  " + std::to_wstring(score) + L" / " + std::to_wstring(MatchRule::kScoreToWin);
}

std::wstring FormatRedScore(uint8_t score)
{
	return std::to_wstring(score) + L" / " + std::to_wstring(MatchRule::kScoreToWin) + L"  RED";
}
} // namespace

void WorldSceneControllerComponent::OnStart()
{
	CreateTeamScoreUI();
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	CreateRestirDebugOverlayUI();
#endif
}

void WorldSceneControllerComponent::OnUpdate(float deltaTime)
{
	if (m_scoreTextDirty)
	{
		m_scoreTextDirty = !RefreshTeamScoreText();
	}
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	RefreshRestirDebugOverlay();
#endif

#ifdef APPLY_LOBBY_SERVER
	if (GLOBAL(InputGlobal).GetInputDown('L'))
	{
		GLOBAL(NetBridge::NetworkGlobal).DisconnectGameServer();
		GLOBAL(NetBridge::NetworkGlobal).ReconnectLobbyServer();
		auto pb{NetBridge::C2S::Make_CL_RETURN_TO_GAME_ROOM_PACKET(GLOBAL(SceneGlobal).GetSessionID())};
		GLOBAL(NetBridge::NetworkGlobal).SendLobby(std::move(pb));
	}
#endif
}

#if defined(ENABLE_RENDER_DEBUG_VIEWS)
void WorldSceneControllerComponent::CreateRestirDebugOverlayUI()
{
	auto* scene = GetGameObject()->GetScene();
	if (!scene)
	{
		return;
	}

	scene->ReserveGameObject(
		"RestirDebugOverlay", std::nullopt,
		[this, scene](GameObject* obj)
		{
			scene->CreateComponentWithInit<RectTransformComponent>(
				obj->GetHandle(),
				[](RectTransformComponent* rect)
				{
					rect->SetAnchors({1.0f, 0.0f}, {1.0f, 0.0f});
					rect->SetPivot({1.0f, 0.0f});
					rect->SetOffsetMin({-470.0f, 18.0f});
					rect->SetOffsetMax({-18.0f, 132.0f});
				}
			);

			scene->CreateComponentWithInit<ImageUIComponent>(
				obj->GetHandle(),
				[](ImageUIComponent* image)
				{
					auto texture = GLOBAL(ResourceGlobal).Load<TextureResource>(L"Resource\\Texture\\UIback.evtex");
					image->SetNormalTextureResource(texture);
					image->SetNormalColor({0.18f, 0.16f, 0.12f, 0.92f});
					image->SetOrder(100010);
				}
			);

			m_restirDebugTextHandle = scene->CreateComponentWithInit<TextUIComponent>(
				obj->GetHandle(),
				[](TextUIComponent* text)
				{
					text->SetText(L"");
					text->SetFontSize(16.0f);
					text->SetHorizontalAlign(TextHorizontalAlign::Left);
					text->SetVerticalAlign(TextVerticalAlign::Center);
					text->SetColor({0.93f, 0.86f, 0.65f, 1.0f});
					text->SetOrder(100011);
				}
			);
			obj->SetActive(false);
		}
	);
}

bool WorldSceneControllerComponent::RefreshRestirDebugOverlay()
{
	auto* scene = GetGameObject()->GetScene();
	if (!scene)
	{
		return false;
	}

	auto* textStorage = scene->GetStorage<TextUIComponent>();
	if (!textStorage)
	{
		return false;
	}

	auto* text = textStorage->Get(m_restirDebugTextHandle);
	if (!text)
	{
		return false;
	}

	auto& debug = GLOBAL(RestirDebugGlobal);
	if (m_lastRestirDebugRevision == debug.GetRevision())
	{
		return true;
	}

	auto* owner = text->GetGameObject();
	if (!owner)
	{
		return false;
	}

	const bool visible = debug.IsOverrideActive();
	owner->SetActive(visible);
	if (visible)
	{
		std::wstring status = L"  SOURCE : ";
		status += debug.GetSourceName();
		status += L"\n  VIEW   : ";
		status += debug.GetViewName();
		status += L"\n  DLSS   : ";
		status += debug.BypassDlss() ? L"BYPASSED" : L"REQUESTED";
		status += L"\n  F12 SOURCE  |  F10 VIEW  |  SHIFT REVERSE";
		text->SetText(std::move(status));
	}
	m_lastRestirDebugRevision = debug.GetRevision();
	return true;
}
#endif

void WorldSceneControllerComponent::SetTeamScores(uint8_t blueScore, uint8_t redScore)
{
	m_blueScore = blueScore;
	m_redScore = redScore;
	m_scoreTextDirty = true;
}

void WorldSceneControllerComponent::CreateTeamScoreUI()
{
	auto* scene = GetGameObject()->GetScene();
	if (!scene)
	{
		return;
	}

	scene->ReserveGameObject(
		"TeamScorePanel", std::nullopt,
		[scene](GameObject* obj)
		{
			scene->CreateComponentWithInit<RectTransformComponent>(
				obj->GetHandle(),
				[](RectTransformComponent* rect)
				{
					rect->SetAnchors({0.5f, 0.0f}, {0.5f, 0.0f});
					rect->SetPivot({0.5f, 0.0f});
					rect->SetOffsetMin({-360.0f, 16.0f});
					rect->SetOffsetMax({360.0f, 60.0f});
				}
			);

			scene->CreateComponentWithInit<ImageUIComponent>(
				obj->GetHandle(),
				[](ImageUIComponent* image)
				{
					auto texture = GLOBAL(ResourceGlobal).Load<TextureResource>(L"Resource\\Texture\\UIback.evtex");
					image->SetNormalTextureResource(texture);
					image->SetNormalColor({1.0f, 1.0f, 1.0f, 0.95f});
					image->SetOrder(99994);
				}
			);
		}
	);

	scene->ReserveGameObject(
		"BlueTeamScoreText", std::nullopt,
		[this, scene](GameObject* obj)
		{
			scene->CreateComponentWithInit<RectTransformComponent>(
				obj->GetHandle(),
				[](RectTransformComponent* rect)
				{
					rect->SetAnchors({0.5f, 0.0f}, {0.5f, 0.0f});
					rect->SetPivot({0.5f, 0.0f});
					rect->SetOffsetMin({-344.0f, 16.0f});
					rect->SetOffsetMax({-72.0f, 60.0f});
				}
			);

			m_blueTeamScoreTextHandle = scene->CreateComponentWithInit<TextUIComponent>(
				obj->GetHandle(),
				[this](TextUIComponent* text)
				{
					text->SetText(FormatBlueScore(m_blueScore));
					text->SetFontSize(23.0f);
					text->SetHorizontalAlign(TextHorizontalAlign::Center);
					text->SetVerticalAlign(TextVerticalAlign::Center);
					text->SetColor({0.35f, 0.55f, 1.0f, 1.0f});
					text->SetOrder(99995);
				}
			);
		}
	);

	scene->ReserveGameObject(
		"TeamScoreDivider", std::nullopt,
		[scene](GameObject* obj)
		{
			scene->CreateComponentWithInit<RectTransformComponent>(
				obj->GetHandle(),
				[](RectTransformComponent* rect)
				{
					rect->SetAnchors({0.5f, 0.0f}, {0.5f, 0.0f});
					rect->SetPivot({0.5f, 0.5f});
					rect->SetOffsetMin({-56.0f, 37.0f});
					rect->SetOffsetMax({56.0f, 39.0f});
				}
			);

			scene->CreateComponentWithInit<ImageUIComponent>(
				obj->GetHandle(),
				[](ImageUIComponent* image)
				{
					image->SetNormalColor({0.85f, 0.72f, 0.32f, 0.9f});
					image->SetOrder(99995);
				}
			);
		}
	);

	scene->ReserveGameObject(
		"RedTeamScoreText", std::nullopt,
		[this, scene](GameObject* obj)
		{
			scene->CreateComponentWithInit<RectTransformComponent>(
				obj->GetHandle(),
				[](RectTransformComponent* rect)
				{
					rect->SetAnchors({0.5f, 0.0f}, {0.5f, 0.0f});
					rect->SetPivot({0.5f, 0.0f});
					rect->SetOffsetMin({72.0f, 16.0f});
					rect->SetOffsetMax({344.0f, 60.0f});
				}
			);

			m_redTeamScoreTextHandle = scene->CreateComponentWithInit<TextUIComponent>(
				obj->GetHandle(),
				[this](TextUIComponent* text)
				{
					text->SetText(FormatRedScore(m_redScore));
					text->SetFontSize(23.0f);
					text->SetHorizontalAlign(TextHorizontalAlign::Center);
					text->SetVerticalAlign(TextVerticalAlign::Center);
					text->SetColor({1.0f, 0.35f, 0.35f, 1.0f});
					text->SetOrder(99995);
				}
			);
		}
	);
}

bool WorldSceneControllerComponent::RefreshTeamScoreText()
{
	auto* scene = GetGameObject()->GetScene();
	if (!scene)
	{
		return false;
	}

	auto* textStorage = scene->GetStorage<TextUIComponent>();
	if (!textStorage)
	{
		return false;
	}

	auto* blueText = textStorage->Get(m_blueTeamScoreTextHandle);
	auto* redText = textStorage->Get(m_redTeamScoreTextHandle);
	if (!blueText || !redText)
	{
		return false;
	}

	blueText->SetText(FormatBlueScore(m_blueScore));
	redText->SetText(FormatRedScore(m_redScore));
	return true;
}
