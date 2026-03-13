#include "stdafxClient.h"
#include "RoomSceneControllerComponent.h"

#include "InputGlobal.h"
#include "NetworkGlobal.h"
#include "Packets/C2SPackets.h"

void RoomSceneControllerComponent::OnUpdate(float deltaTime)
{
	// TODO: 방 나가기(로비로 이동)
	if(GLOBAL(InputGlobal).GetInputDown(VK_F1)) {
		auto pb{NetBridge::C2S::Make_CS_LEAVE_GAME_ROOM_PACKET()};
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}

	// TODO: 팀 변경
	if (GLOBAL(InputGlobal).GetInputDown(VK_F2)) {
		auto pb{NetBridge::C2S::Make_CS_CHANGE_TEAM_PACKET()};
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}
	
	// TODO: 준비해제/준비완료
	if (GLOBAL(InputGlobal).GetInputDown(VK_F3)) {
		auto pb{NetBridge::C2S::Make_CS_READY_GAME_PACKET()};
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}

	// TODO: 봇 추가/봇 삭제
	if (GLOBAL(InputGlobal).GetInputDown(VK_F4)) {
		auto pb{NetBridge::C2S::Make_CS_ADD_BOT_PACKET(FB_ENUMS::TEAM_TYPE_OFFENSE)};
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}
	
	// TODO: 게임 시작하기
	if (GLOBAL(InputGlobal).GetInputDown(VK_F5)) {
		auto pb{NetBridge::C2S::Make_CS_START_GAME_PACKET()};
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}
}