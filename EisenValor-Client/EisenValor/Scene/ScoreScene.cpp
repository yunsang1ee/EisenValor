#include "stdafxClient.h"
#include "ScoreScene.h"

#include "AudioGlobal.h"
#include "ButtonUIComponent.h"
#include "Component/Lobby/LobbyClientState.h"
#include "ImageUIComponent.h"
#include "InputGlobal.h"
#include "NetworkGlobal.h"
#include "Packets/C2SPackets.h"
#include "RectTransformComponent.h"
#include "ResourceGlobal.h"
#include "SceneGlobal.h"
#include "TextureResource.h"
#include "Util/GameConstants.h"

FB_ENUMS::TEAM_TYPE ScoreScene::s_winningTeam = FB_ENUMS::TEAM_TYPE_NONE;
uint8				ScoreScene::s_redScore = 0;
uint8				ScoreScene::s_blueScore = 0;

void ScoreScene::SetResult(FB_ENUMS::TEAM_TYPE winningTeam, uint8 blueScore, uint8 redScore)
{
	s_winningTeam = winningTeam;
	s_blueScore = blueScore;
	s_redScore = redScore;
}

void ScoreScene::OnRegisterCustomComponents() {}

void ScoreScene::OnStartImpl()
{
	GLOBAL(InputGlobal).SetMouseLocked(false);
	m_returnPending = false;

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
					auto texture = GLOBAL(ResourceGlobal).Load<TextureResource>(L"Resource\\Texture\\scorescene.evtex");
					image->SetNormalTextureResource(texture);
					image->SetNormalColor({1.0f, 1.0f, 1.0f, 1.0f});
					image->SetOrder(0);
				}
			);
		}
	);

	ReserveGameObject(
		"ScoreScenePanel", std::nullopt,
		[this](GameObject* obj)
		{
			CreateComponentWithInit<RectTransformComponent>(
				obj->GetHandle(),
				[](RectTransformComponent* rect)
				{
					rect->SetAnchors({0.5f, 0.5f}, {0.5f, 0.5f});
					rect->SetPivot({0.5f, 0.5f});
					rect->SetOffsetMin({-540.0f, -350.0f});
					rect->SetOffsetMax({540.0f, 350.0f});
				}
			);
			CreateComponentWithInit<ImageUIComponent>(
				obj->GetHandle(),
				[](ImageUIComponent* image)
				{
					image->SetNormalColor({0.025f, 0.025f, 0.025f, 0.9f});
					image->SetOrder(10);
				}
			);
		}
	);

	const auto reserveText = [this](
								 const char* name, Vec2 offsetMin, Vec2 offsetMax, std::wstring value, float fontSize,
								 DirectX::XMFLOAT4 color, int32 order = 30
							 )
	{
		ReserveGameObject(
			name, std::nullopt,
			[this, offsetMin, offsetMax, value = std::move(value), fontSize, color, order](GameObject* obj)
			{
				CreateComponentWithInit<RectTransformComponent>(
					obj->GetHandle(),
					[offsetMin, offsetMax](RectTransformComponent* rect)
					{
						rect->SetAnchors({0.5f, 0.5f}, {0.5f, 0.5f});
						rect->SetPivot({0.5f, 0.5f});
						rect->SetOffsetMin(offsetMin);
						rect->SetOffsetMax(offsetMax);
					}
				);
				CreateComponentWithInit<TextUIComponent>(
					obj->GetHandle(),
					[value, fontSize, color, order](TextUIComponent* text)
					{
						text->SetText(value);
						text->SetFontSize(fontSize);
						text->SetHorizontalAlign(TextHorizontalAlign::Center);
						text->SetVerticalAlign(TextVerticalAlign::Center);
						text->SetColor(color);
						text->SetOrder(order);
					}
				);
			}
		);
	};

	std::wstring	  headline = L"BATTLE COMPLETE";
	DirectX::XMFLOAT4 headlineColor{0.9f, 0.75f, 0.3f, 1.0f};
	const auto*		  localParticipant = GLOBAL(LobbyClientState).GetLocalParticipant();
	if (s_winningTeam == FB_ENUMS::TEAM_TYPE_NONE)
	{
		headline = L"DRAW";
		headlineColor = {0.85f, 0.85f, 0.85f, 1.0f};
	}
	else if (localParticipant && localParticipant->team == s_winningTeam)
	{
		headline = L"VICTORY";
		headlineColor = {0.9f, 0.72f, 0.2f, 1.0f};
	}
	else if (localParticipant && localParticipant->team != FB_ENUMS::TEAM_TYPE_NONE)
	{
		headline = L"DEFEAT";
		headlineColor = {0.7f, 0.72f, 0.78f, 1.0f};
	}

	std::wstring winnerText = L"NO TEAM CLAIMED THE FIELD";
	if (s_winningTeam == FB_ENUMS::TEAM_TYPE_BLUE)
	{
		winnerText = L"BLUE TEAM WINS";
	}
	else if (s_winningTeam == FB_ENUMS::TEAM_TYPE_RED)
	{
		winnerText = L"RED TEAM WINS";
	}

	reserveText("ScoreResultHeadline", {-480.0f, -285.0f}, {480.0f, -195.0f}, headline, 58.0f, headlineColor, 31);
	reserveText(
		"ScoreWinnerText", {-480.0f, -195.0f}, {480.0f, -135.0f}, winnerText, 24.0f, {0.85f, 0.8f, 0.68f, 1.0f}
	);
	reserveText(
		"ScoreRedTeamLabel", {-470.0f, -75.0f}, {-60.0f, -20.0f}, L"RED TEAM", 27.0f, {1.0f, 0.25f, 0.25f, 1.0f}
	);
	reserveText(
		"ScoreRedTeamValue", {-470.0f, -15.0f}, {-60.0f, 105.0f}, std::to_wstring(s_redScore), 76.0f,
		{1.0f, 0.35f, 0.35f, 1.0f}
	);
	reserveText("ScoreVersusText", {-60.0f, -20.0f}, {60.0f, 100.0f}, L"VS", 30.0f, {0.8f, 0.72f, 0.55f, 1.0f});
	reserveText(
		"ScoreBlueTeamLabel", {60.0f, -75.0f}, {470.0f, -20.0f}, L"BLUE TEAM", 27.0f, {0.2f, 0.45f, 1.0f, 1.0f}
	);
	reserveText(
		"ScoreBlueTeamValue", {60.0f, -15.0f}, {470.0f, 105.0f}, std::to_wstring(s_blueScore), 76.0f,
		{0.3f, 0.55f, 1.0f, 1.0f}
	);

	ReserveGameObject(
		"ScoreReturnStatusText", std::nullopt,
		[this](GameObject* obj)
		{
			CreateComponentWithInit<RectTransformComponent>(
				obj->GetHandle(),
				[](RectTransformComponent* rect)
				{
					rect->SetAnchors({0.5f, 0.5f}, {0.5f, 0.5f});
					rect->SetPivot({0.5f, 0.5f});
					rect->SetOffsetMin({-440.0f, 125.0f});
					rect->SetOffsetMax({440.0f, 175.0f});
				}
			);
			m_statusTextHandle = CreateComponentWithInit<TextUIComponent>(
				obj->GetHandle(),
				[](TextUIComponent* text)
				{
					text->SetText(L"");
					text->SetFontSize(19.0f);
					text->SetHorizontalAlign(TextHorizontalAlign::Center);
					text->SetVerticalAlign(TextVerticalAlign::Center);
					text->SetColor({0.85f, 0.8f, 0.68f, 1.0f});
					text->SetOrder(31);
				}
			);
		}
	);

	ReserveGameObject(
		"ScoreReturnButton", std::nullopt,
		[this](GameObject* obj)
		{
			CreateComponentWithInit<RectTransformComponent>(
				obj->GetHandle(),
				[](RectTransformComponent* rect)
				{
					rect->SetAnchors({0.5f, 0.5f}, {0.5f, 0.5f});
					rect->SetPivot({0.5f, 0.5f});
					rect->SetOffsetMin({-260.0f, 195.0f});
					rect->SetOffsetMax({260.0f, 265.0f});
				}
			);
			auto imageHandle = CreateComponentWithInit<ImageUIComponent>(
				obj->GetHandle(),
				[](ImageUIComponent* image)
				{
					image->SetNormalColor({0.06f, 0.06f, 0.06f, 0.95f});
					image->SetHoverColor({0.55f, 0.4f, 0.12f, 1.0f});
					image->SetPressedColor({0.42f, 0.28f, 0.08f, 1.0f});
					image->SetDisabledColor({0.05f, 0.05f, 0.05f, 0.9f});
					image->SetOrder(20);
				}
			);
			CreateComponentWithInit<TextUIComponent>(
				obj->GetHandle(),
				[](TextUIComponent* text)
				{
					text->SetText(L"RETURN TO BATTLE ROOM");
					text->SetFontSize(22.0f);
					text->SetHorizontalAlign(TextHorizontalAlign::Center);
					text->SetVerticalAlign(TextVerticalAlign::Center);
					text->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
					text->SetOrder(21);
				}
			);
			CreateComponentWithInit<ButtonUIComponent>(
				obj->GetHandle(),
				[this, imageHandle](ButtonUIComponent* button)
				{
					button->SetTargetImage(imageHandle);
					button->SetOnHover(
						[]()
						{
							GLOBAL(AudioGlobal)
								.Play2D(
									L"Resource/Sounds/click.wav", AudioBus::UI, false, AudioBalance::kUIButtonVolume
								);
						}
					);
					button->SetOnClick(
						[this]()
						{
							GLOBAL(AudioGlobal)
								.Play2D(
									L"Resource/Sounds/mouseclick.wav", AudioBus::UI, false,
									AudioBalance::kUIButtonVolume
								);
							ReturnToRoom();
						}
					);
				}
			);
		}
	);
}

void ScoreScene::OnEndImpl()
{
	m_returnPending = false;
}

void ScoreScene::ReturnToRoom()
{
	if (m_returnPending)
	{
		return;
	}

	if (false == GLOBAL(NetBridge::NetworkGlobal).ReconnectLobbyServer())
	{
		SetStatusText(L"FAILED TO CONNECT. SELECT RETURN TO RETRY.");
		return;
	}

	m_returnPending = true;
	SetStatusText(L"RETURNING TO BATTLE ROOM...");
	auto pb{NetBridge::C2S::Make_CL_RETURN_TO_GAME_ROOM_PACKET(GLOBAL(SceneGlobal).GetSessionID())};
	GLOBAL(NetBridge::NetworkGlobal).SendLobby(std::move(pb));
}

void ScoreScene::SetStatusText(const std::wstring& message)
{
	auto* storage = GetStorage<TextUIComponent>();
	if (!storage)
	{
		return;
	}

	if (auto* text = storage->Get(m_statusTextHandle))
	{
		text->SetText(message);
	}
}
