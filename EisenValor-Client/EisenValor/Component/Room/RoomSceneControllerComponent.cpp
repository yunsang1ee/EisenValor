#include "stdafxClient.h"
#include "RoomSceneControllerComponent.h"

#include "AudioGlobal.h"
#include "ButtonUIComponent.h"
#include "Component/Lobby/LobbyClientState.h"
#include "NetworkGlobal.h"
#include "Packets/C2SPackets.h"
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

std::wstring MakeParticipantLabel(const RoomParticipantView& participant, uint32 localID)
{
	std::wstring label;
	if (participant.type == FB_ENUMS::PARTICIPANT_TYPE_HOST)
	{
		label = L"HOST #";
	}
	else if (participant.type == FB_ENUMS::PARTICIPANT_TYPE_BOT)
	{
		label = L"BOT #";
	}
	else
	{
		label = L"PLAYER #";
	}

	label += std::to_wstring(participant.id);
	if (participant.id == localID)
	{
		label += L"  (YOU)";
	}
	if (participant.type == FB_ENUMS::PARTICIPANT_TYPE_BOT)
	{
		label += L"        REMOVE";
	}
	else if (participant.type != FB_ENUMS::PARTICIPANT_TYPE_HOST)
	{
		label += participant.state == FB_ENUMS::PARTICIPANT_STATE_TYPE_READY ? L"        READY" : L"        NOT READY";
	}
	return label;
}
} // namespace

void RoomSceneControllerComponent::OnUpdate(float deltaTime)
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

	const auto* localParticipant = state.GetLocalParticipant();
	const bool isHost = localParticipant && localParticipant->type == FB_ENUMS::PARTICIPANT_TYPE_HOST;
	std::vector<const RoomParticipantView*> blueParticipants;
	std::vector<const RoomParticipantView*> redParticipants;
	for (const auto& participant : state.GetParticipants())
	{
		if (participant.team == FB_ENUMS::TEAM_TYPE_BLUE)
		{
			blueParticipants.push_back(&participant);
		}
		else if (participant.team == FB_ENUMS::TEAM_TYPE_RED)
		{
			redParticipants.push_back(&participant);
		}
	}

	for (auto& text : scene->GetStorage<TextUIComponent>()->GetList())
	{
		auto* owner = text.GetGameObject();
		if (!owner)
		{
			continue;
		}

		const auto& name = owner->GetName();
		if (name == "RoomMessageText")
		{
			text.SetText(std::wstring(state.GetStatusMessage().begin(), state.GetStatusMessage().end()));
			continue;
		}
		if (name == "RoomReadyButton")
		{
			text.SetText(
				localParticipant && localParticipant->state == FB_ENUMS::PARTICIPANT_STATE_TYPE_READY ? L"CANCEL READY"
																									  : L"READY"
			);
			SetButtonEnabled(owner->GetComponent<ButtonUIComponent>(), localParticipant && !isHost);
			continue;
		}
		if (name == "RoomStartButton")
		{
			text.SetText(state.IsStartPending() ? L"STARTING..." : L"START GAME");
			SetButtonEnabled(owner->GetComponent<ButtonUIComponent>(), isHost && !state.IsStartPending());
			continue;
		}
		if (name == "RoomAddBlueButton" || name == "RoomAddRedButton")
		{
			SetButtonEnabled(owner->GetComponent<ButtonUIComponent>(), isHost && state.GetParticipants().size() < 6);
			continue;
		}

		const bool isBlueRow = name.starts_with("RoomBlueRow");
		const bool isRedRow = name.starts_with("RoomRedRow");
		if (!isBlueRow && !isRedRow)
		{
			continue;
		}

		const auto&	 participants = isBlueRow ? blueParticipants : redParticipants;
		const size_t rowIndex = static_cast<size_t>(name.back() - '0');
		if (rowIndex >= participants.size())
		{
			owner->SetActive(false);
			continue;
		}

		owner->SetActive(true);
		const auto* participant = participants[rowIndex];
		text.SetText(MakeParticipantLabel(*participant, state.GetLocalParticipantID()));
		auto*	   button = owner->GetComponent<ButtonUIComponent>();
		const bool canRemove = isHost && participant->type == FB_ENUMS::PARTICIPANT_TYPE_BOT;
		SetButtonEnabled(button, canRemove);
		if (button)
		{
			const uint32 participantID = participant->id;
			button->SetOnClick(
				[participantID]()
				{
					GLOBAL(AudioGlobal).Play2D(
						L"Resource/Sounds/mouseclick.wav", AudioBus::UI, false, 0.05f
					);
					auto pb{NetBridge::C2S::Make_CL_REMOVE_BOT_PACKET(participantID)};
					GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
				}
			);
		}
	}
}
