#include "stdafxClient.h"
#include "LobbyScene.h"

#include "AudioGlobal.h"
#include "ButtonUIComponent.h"
#include "Component/Lobby/LobbyClientState.h"
#include "Component/Lobby/LobbySceneControllerComponent.h"
#include "ImageUIComponent.h"
#include "NetworkGlobal.h"
#include "Packets/C2SPackets.h"
#include "RectTransformComponent.h"
#include "ResourceGlobal.h"
#include "TextUIComponent.h"
#include "TextureResource.h"
#include "Util/GameConstants.h"

void LobbyScene::OnRegisterCustomComponents()
{
	RegisterComponent<LobbySceneControllerComponent>();
}

void LobbyScene::OnStartImpl()
{
	DEBUG_LOG_FMT("[LobbyScene] Enter Lobby Scene.\n");

	ReserveGameObject(
		"LobbySceneBackground", std::nullopt,
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
		"LobbyPanel", std::nullopt,
		[this](GameObject* obj)
		{
			CreateComponentWithInit<RectTransformComponent>(
				obj->GetHandle(),
				[](RectTransformComponent* rect)
				{
					rect->SetAnchors({0.5f, 0.5f}, {0.5f, 0.5f});
					rect->SetPivot({0.5f, 0.5f});
					rect->SetOffsetMin({-570.0f, -390.0f});
					rect->SetOffsetMax({570.0f, 390.0f});
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
						   TextHorizontalAlign align = TextHorizontalAlign::Left
					   )
	{
		ReserveGameObject(
			name, std::nullopt,
			[this, offsetMin, offsetMax, value, fontSize, align](GameObject* obj)
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
					[value, fontSize, align](TextUIComponent* text)
					{
						text->SetText(value);
						text->SetFontSize(fontSize);
						text->SetHorizontalAlign(align);
						text->SetVerticalAlign(TextVerticalAlign::Center);
						text->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
						text->SetOrder(30);
					}
				);
			}
		);
	};

	auto reserveButton =
		[this](const char* name, Vec2 offsetMin, Vec2 offsetMax, const wchar_t* label, std::function<void()> onClick)
	{
		ReserveGameObject(
			name, std::nullopt,
			[this, offsetMin, offsetMax, label, onClick = std::move(onClick)](GameObject* obj)
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
					[](ImageUIComponent* image)
					{
						image->SetNormalColor({0.05f, 0.05f, 0.05f, 0.9f});
						image->SetHoverColor({0.15f, 0.35f, 1.0f, 1.0f});
						image->SetPressedColor({0.15f, 0.35f, 1.0f, 1.0f});
						image->SetDisabledColor({0.05f, 0.05f, 0.05f, 0.9f});
						image->SetOrder(20);
					}
				);
				CreateComponentWithInit<TextUIComponent>(
					obj->GetHandle(),
					[label](TextUIComponent* text)
					{
						text->SetText(label);
						text->SetFontSize(24.0f);
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
							{
								GLOBAL(AudioGlobal).Play2D(
									L"Resource/Sounds/click.wav", AudioBus::UI, false, AudioBalance::kUIButtonVolume
								);
							}
						);
						button->SetOnClick(
							[onClick = std::move(onClick)]()
							{
								GLOBAL(AudioGlobal)
									.Play2D(
										L"Resource/Sounds/mouseclick.wav", AudioBus::UI, false,
										AudioBalance::kUIButtonVolume
									);
								onClick();
							}
						);
					}
				);
			}
		);
	};

	reserveText("LobbyTitleText", {-500.0f, -330.0f}, {500.0f, -250.0f}, L"LOBBY", 40.0f);
	reserveText("LobbySummaryText", {-500.0f, -250.0f}, {500.0f, -200.0f}, L"PLAYERS  0     ROOMS  0", 22.0f);
	reserveText("LobbyMessageText", {-500.0f, 350.0f}, {500.0f, 385.0f}, L"", 18.0f);

	reserveButton(
		"LobbyCreateRoomButton", {250.0f, -250.0f}, {500.0f, -190.0f}, L"CREATE ROOM",
		[]()
		{
			auto pb{NetBridge::C2S::Make_CL_MAKE_GAME_ROOM_PACKET()};
			GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
		}
	);

	for (size_t rowIndex = 0; rowIndex < 6; ++rowIndex)
	{
		const std::string name = "LobbyRoomRow" + std::to_string(rowIndex);
		const float		  top = -160.0f + static_cast<float>(rowIndex) * 72.0f;
		reserveButton(
			name.c_str(), {-500.0f, top}, {500.0f, top + 58.0f}, L"",
			[rowIndex]()
			{
				auto&		 state = GLOBAL(LobbyClientState);
				const auto&	 rooms = state.GetRooms();
				const size_t roomIndex = state.GetLobbyPage() * LobbyClientState::kRoomsPerPage + rowIndex;
				if (roomIndex >= rooms.size())
				{
					return;
				}
				auto pb{NetBridge::C2S::Make_CL_ENTER_GAME_ROOM_PACKET(rooms[roomIndex].id)};
				GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
			}
		);
	}

	reserveButton(
		"LobbyPreviousPageButton", {-240.0f, 300.0f}, {-80.0f, 350.0f}, L"PREV",
		[]() { GLOBAL(LobbyClientState).PreviousLobbyPage(); }
	);
	reserveText("LobbyPageText", {-80.0f, 300.0f}, {80.0f, 350.0f}, L"1 / 1", 20.0f, TextHorizontalAlign::Center);
	reserveButton(
		"LobbyNextPageButton", {80.0f, 300.0f}, {240.0f, 350.0f}, L"NEXT",
		[]() { GLOBAL(LobbyClientState).NextLobbyPage(); }
	);

	reserveButton(
		"LobbyExitButton", {300.0f, 300.0f}, {500.0f, 350.0f}, L"LOG OUT",
		[]()
		{
			auto pb{NetBridge::C2S::Make_CL_LEAVE_GAME_LOBBY_PACKET()};
			GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
		}
	);

	ReserveGameObject(
		"LobbySceneController", std::nullopt,
		[this](GameObject* obj) { CreateComponent<LobbySceneControllerComponent>(obj->GetHandle()); }
	);
}

void LobbyScene::OnEndImpl() {}
