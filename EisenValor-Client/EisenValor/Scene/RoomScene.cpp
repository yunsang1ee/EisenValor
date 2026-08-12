#include "stdafxClient.h"
#include "RoomScene.h"

#include "AudioGlobal.h"
#include "ButtonUIComponent.h"
#include "Component/Lobby/LobbyClientState.h"
#include "Component/Room/RoomSceneControllerComponent.h"
#include "ImageUIComponent.h"
#include "NetworkGlobal.h"
#include "Packets/C2SPackets.h"
#include "RectTransformComponent.h"
#include "ResourceGlobal.h"
#include "TextUIComponent.h"
#include "TextureResource.h"

void RoomScene::OnRegisterCustomComponents()
{
	RegisterComponent<RoomSceneControllerComponent>();
}

void RoomScene::OnStartImpl()
{
	DEBUG_LOG_FMT("[RoomScene] Enter Room Scene.\n");

	ReserveGameObject(
		"RoomSceneBackground", std::nullopt,
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
					auto texture =
						GLOBAL(ResourceGlobal).Load<TextureResource>(L"Resource\\Texture\\Scene\\startscene.evtex");
					image->SetNormalTextureResource(texture);
					image->SetNormalColor({1.0f, 1.0f, 1.0f, 1.0f});
					image->SetOrder(0);
				}
			);
		}
	);

	ReserveGameObject(
		"RoomPanel", std::nullopt,
		[this](GameObject* obj)
		{
			CreateComponentWithInit<RectTransformComponent>(
				obj->GetHandle(),
				[](RectTransformComponent* rect)
				{
					rect->SetAnchors({0.5f, 0.5f}, {0.5f, 0.5f});
					rect->SetPivot({0.5f, 0.5f});
					rect->SetOffsetMin({-650.0f, -400.0f});
					rect->SetOffsetMax({650.0f, 400.0f});
				}
			);
			CreateComponentWithInit<ImageUIComponent>(
				obj->GetHandle(),
				[](ImageUIComponent* image)
				{
					image->SetNormalColor({0.05f, 0.05f, 0.05f, 0.9f});
					image->SetOrder(10);
				}
			);
		}
	);

	auto reserveText = [this](
						   const char* name, Vec2 offsetMin, Vec2 offsetMax, const wchar_t* value, float fontSize,
						   DirectX::XMFLOAT4 color = {1.0f, 1.0f, 1.0f, 1.0f}
					   )
	{
		ReserveGameObject(
			name, std::nullopt,
			[this, offsetMin, offsetMax, value, fontSize, color](GameObject* obj)
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
					[value, fontSize, color](TextUIComponent* text)
					{
						text->SetText(value);
						text->SetFontSize(fontSize);
						text->SetHorizontalAlign(TextHorizontalAlign::Center);
						text->SetVerticalAlign(TextVerticalAlign::Center);
						text->SetColor(color);
						text->SetOrder(30);
					}
				);
			}
		);
	};

	auto reserveButton = [this](
							 const char* name, Vec2 offsetMin, Vec2 offsetMax, const wchar_t* label,
							 std::function<void()> onClick, DirectX::XMFLOAT4 hoverColor = {0.15f, 0.35f, 1.0f, 1.0f}
						 )
	{
		ReserveGameObject(
			name, std::nullopt,
			[this, offsetMin, offsetMax, label, onClick = std::move(onClick), hoverColor](GameObject* obj)
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
				auto imageHandle = CreateComponentWithInit<ImageUIComponent>(
					obj->GetHandle(),
					[hoverColor](ImageUIComponent* image)
					{
						image->SetNormalColor({0.05f, 0.05f, 0.05f, 0.9f});
						image->SetHoverColor(hoverColor);
						image->SetPressedColor(hoverColor);
						image->SetDisabledColor({0.05f, 0.05f, 0.05f, 0.9f});
						image->SetOrder(20);
					}
				);
				CreateComponentWithInit<TextUIComponent>(
					obj->GetHandle(),
					[label](TextUIComponent* text)
					{
						text->SetText(label);
						text->SetFontSize(21.0f);
						text->SetHorizontalAlign(TextHorizontalAlign::Center);
						text->SetVerticalAlign(TextVerticalAlign::Center);
						text->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
						text->SetOrder(21);
					}
				);
				CreateComponentWithInit<ButtonUIComponent>(
					obj->GetHandle(),
					[imageHandle, onClick](ButtonUIComponent* button)
					{
						button->SetTargetImage(imageHandle);
						button->SetOnHover(
							[]()
							{ GLOBAL(AudioGlobal).Play2D(L"Resource/Sounds/click.wav", AudioBus::UI, false, 0.05f); }
						);
						button->SetOnClick(
							[onClick = std::move(onClick)]()
							{
								GLOBAL(AudioGlobal)
									.Play2D(L"Resource/Sounds/mouseclick.wav", AudioBus::UI, false, 0.05f);
								onClick();
							}
						);
					}
				);
			}
		);
	};

	reserveText("RoomTitleText", {-580.0f, -350.0f}, {580.0f, -285.0f}, L"EISENVALOR  /  BATTLE ROOM", 38.0f);
	reserveText(
		"RoomBlueTitle", {-580.0f, -270.0f}, {-40.0f, -215.0f}, L"BLUE TEAM", 28.0f, {0.15f, 0.35f, 1.0f, 1.0f}
	);
	reserveText("RoomRedTitle", {40.0f, -270.0f}, {580.0f, -215.0f}, L"RED TEAM", 28.0f, {1.0f, 0.2f, 0.2f, 1.0f});
	reserveText("RoomMessageText", {-580.0f, 220.0f}, {580.0f, 270.0f}, L"", 20.0f);

	for (size_t rowIndex = 0; rowIndex < 3; ++rowIndex)
	{
		const float		  top = -190.0f + static_cast<float>(rowIndex) * 90.0f;
		const std::string blueName = "RoomBlueRow" + std::to_string(rowIndex);
		const std::string redName = "RoomRedRow" + std::to_string(rowIndex);
		reserveButton(blueName.c_str(), {-580.0f, top}, {-40.0f, top + 68.0f}, L"", []() {});
		reserveButton(redName.c_str(), {40.0f, top}, {580.0f, top + 68.0f}, L"", []() {}, {1.0f, 0.2f, 0.2f, 1.0f});
	}

	reserveButton(
		"RoomAddBlueButton", {-580.0f, 95.0f}, {-300.0f, 150.0f}, L"ADD BLUE BOT",
		[]()
		{
			auto pb{NetBridge::C2S::Make_CL_ADD_BOT_PACKET(FB_ENUMS::TEAM_TYPE_BLUE)};
			GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
		}
	);
	reserveButton(
		"RoomAddRedButton", {300.0f, 95.0f}, {580.0f, 150.0f}, L"ADD RED BOT",
		[]()
		{
			auto pb{NetBridge::C2S::Make_CL_ADD_BOT_PACKET(FB_ENUMS::TEAM_TYPE_RED)};
			GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
		},
		{1.0f, 0.2f, 0.2f, 1.0f}
	);

	reserveButton(
		"RoomLeaveButton", {-580.0f, 285.0f}, {-350.0f, 345.0f}, L"LEAVE",
		[]()
		{
			auto pb{NetBridge::C2S::Make_CL_LEAVE_GAME_ROOM_PACKET()};
			GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
		}
	);
	reserveButton(
		"RoomTeamButton", {-330.0f, 285.0f}, {-100.0f, 345.0f}, L"CHANGE TEAM",
		[]()
		{
			auto pb{NetBridge::C2S::Make_CL_CHANGE_TEAM_PACKET()};
			GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
		}
	);
	reserveButton(
		"RoomReadyButton", {-80.0f, 285.0f}, {180.0f, 345.0f}, L"READY",
		[]()
		{
			auto pb{NetBridge::C2S::Make_CL_READY_GAME_PACKET()};
			GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
		}
	);
	reserveButton(
		"RoomStartButton", {200.0f, 285.0f}, {580.0f, 345.0f}, L"START GAME",
		[]()
		{
			auto& state = GLOBAL(LobbyClientState);
			if (state.IsStartPending())
			{
				return;
			}
			state.SetStartPending(true);
			auto pb{NetBridge::C2S::Make_CL_START_GAME_PACKET()};
			GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
		}
	);

	ReserveGameObject(
		"RoomSceneController", std::nullopt,
		[this](GameObject* obj) { CreateComponent<RoomSceneControllerComponent>(obj->GetHandle()); }
	);
}

void RoomScene::OnEndImpl() {}
