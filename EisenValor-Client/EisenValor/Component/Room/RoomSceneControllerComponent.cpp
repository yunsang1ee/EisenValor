#include "stdafxClient.h"
#include "RoomSceneControllerComponent.h"

#include "InputGlobal.h"
#include "NetworkGlobal.h"
#include "Packets/C2SPackets.h"

void RoomSceneControllerComponent::OnUpdate(float deltaTime)
{
	// TODO: 방 나가기(로비로 이동)
	if(GLOBAL(InputGlobal).GetInputDown(VK_F1)) {
		auto pb{NetBridge::C2S::Make_CL_LEAVE_GAME_ROOM_PACKET()};
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}

	// TODO: 팀 변경
	if (GLOBAL(InputGlobal).GetInputDown(VK_F2)) {
		auto pb{NetBridge::C2S::Make_CL_CHANGE_TEAM_PACKET()};
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}
	
	// TODO: 준비해제/준비완료
	if (GLOBAL(InputGlobal).GetInputDown(VK_F3)) {
		auto pb{NetBridge::C2S::Make_CL_READY_GAME_PACKET()};
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}

	// TODO: 봇 추가
	if (GLOBAL(InputGlobal).GetInputDown(VK_F4)) {
		static bool flag{false};
		auto pb{NetBridge::C2S::Make_CL_ADD_BOT_PACKET(static_cast<FB_ENUMS::TEAM_TYPE>(flag))};
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
		flag = !flag;
	}
	
	// TODO: 게임 시작하기
	if (GLOBAL(InputGlobal).GetInputDown('R'))
	{
		auto pb{NetBridge::C2S::Make_CL_START_GAME_PACKET()};
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}
}