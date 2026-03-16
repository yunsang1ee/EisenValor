#include "stdafxClient.h"
#include "LobbySceneControllerComponent.h"
#include "InputGlobal.h"
#include "NetworkGlobal.h"
#include "Packets/C2SPackets.h"

void LobbySceneControllerComponent::OnUpdate(float deltaTime)
{
	// 로비에서 나가기
	if (GLOBAL(InputGlobal).GetInputDown(VK_F1))
	{
		auto pb{NetBridge::C2S::Make_CS_LEAVE_GAME_LOBBY_PACKET()};
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}

	// 방 만들기
	if (GLOBAL(InputGlobal).GetInputDown(VK_F2)) {
		auto pb{NetBridge::C2S::Make_CS_MAKE_GAME_ROOM_PACKET()};
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}
	 
	// 방 들어가기
	if (GLOBAL(InputGlobal).GetInputDown(VK_F3)) {
		const uint16 roomID{1};
		auto pb{NetBridge::C2S::Make_CS_ENTER_GAME_ROOM_PACKET(roomID)};
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}
}