#include "stdafxClient.h"
#include "LobbySceneControllerComponent.h"

#include "ButtonUIComponent.h"
#include "Component/Lobby/LobbyClientState.h"
#include "Scene.h"
#include "TextUIComponent.h"

namespace
{
void SetButtonEnabled(ButtonUIComponent* button, bool enabled)
{
	if (!button)
	{
		return;
	}

	button->SetInteractable(enabled);
	button->SetState(enabled ? ButtonState::Normal : ButtonState::Disabled);
}
} // namespace

void LobbySceneControllerComponent::OnUpdate(float deltaTime)
{
	auto& state = GLOBAL(LobbyClientState);
	if (m_lastRevision == state.GetRevision())
	{
		return;
	}
	m_lastRevision = state.GetRevision();

	auto* scene = GetGameObject()->GetScene();
	if (!scene)
	{
		return;
	}

	const auto& rooms = state.GetRooms();
	const size_t pageIndex = state.GetLobbyPage();
	const size_t pageCount = state.GetLobbyPageCount();
	for (auto& text : scene->GetStorage<TextUIComponent>()->GetList())
	{
		auto* owner = text.GetGameObject();
		if (!owner)
		{
			continue;
		}

		const auto& name = owner->GetName();
		if (name == "LobbySummaryText")
		{
			text.SetText(
				L"PLAYERS  " + std::to_wstring(state.GetUserCount()) + L"     ROOMS  " +
				std::to_wstring(rooms.size())
			);
			continue;
		}

		if (name == "LobbyMessageText")
		{
			text.SetText(std::wstring(state.GetStatusMessage().begin(), state.GetStatusMessage().end()));
			continue;
		}

		if (name == "LobbyPageText")
		{
			text.SetText(std::to_wstring(pageIndex + 1) + L" / " + std::to_wstring(pageCount));
			continue;
		}

		if (name == "LobbyPreviousPageButton")
		{
			SetButtonEnabled(owner->GetComponent<ButtonUIComponent>(), pageIndex > 0);
			continue;
		}

		if (name == "LobbyNextPageButton")
		{
			SetButtonEnabled(owner->GetComponent<ButtonUIComponent>(), pageIndex + 1 < pageCount);
			continue;
		}

		constexpr std::string_view prefix = "LobbyRoomRow";
		if (!name.starts_with(prefix))
		{
			continue;
		}

		const size_t rowIndex = static_cast<size_t>(name.back() - '0');
		const size_t roomIndex = pageIndex * LobbyClientState::kRoomsPerPage + rowIndex;
		if (roomIndex >= rooms.size())
		{
			owner->SetActive(false);
			continue;
		}

		owner->SetActive(true);
		const auto&	   room = rooms[roomIndex];
		const wchar_t* roomState = room.state == FB_ENUMS::ROOM_STATE_TYPE_PLAYING ? L"IN GAME" : L"WAITING";
		text.SetText(
			L"ROOM " + std::to_wstring(room.id) + L"     " + roomState + L"     " +
			std::to_wstring(room.currentParticipants) + L" / " + std::to_wstring(room.maxParticipants) + L"        JOIN"
		);
		SetButtonEnabled(
			owner->GetComponent<ButtonUIComponent>(),
			room.state != FB_ENUMS::ROOM_STATE_TYPE_PLAYING && room.currentParticipants < room.maxParticipants
		);
	}
}
