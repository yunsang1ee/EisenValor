#include "stdafxClient.h"
#include "RoomSceneControllerComponent.h"

#include "InputGlobal.h"
#include "NetworkGlobal.h"
#include "Packets/C2SPackets.h"

void RoomSceneControllerComponent::OnUpdate(float deltaTime)
{
	// TODO: 아래 기능들은 나중에 UI와 연동해야 합니다. 현재는 테스트용으로 F1~F4, R 키에 매핑되어 있습니다.
#ifdef APPLY_LOBBY_SERVER
	// 방 나가기(로비로 이동)
	if(GLOBAL(InputGlobal).GetInputDown(VK_F1)) {
		auto pb{NetBridge::C2S::Make_CL_LEAVE_GAME_ROOM_PACKET()};
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}

	// 팀 변경
	if (GLOBAL(InputGlobal).GetInputDown(VK_F3)) {
		auto pb{NetBridge::C2S::Make_CL_CHANGE_TEAM_PACKET()};
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}
	
	// 준비해제/준비완료
	if (GLOBAL(InputGlobal).GetInputDown(VK_F4)) {
		auto pb{NetBridge::C2S::Make_CL_READY_GAME_PACKET()};
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}

	// 봇 추가
	if (GLOBAL(InputGlobal).GetInputDown(VK_F5)) {
		static FB_ENUMS::TEAM_TYPE flag{FB_ENUMS::TEAM_TYPE_BLUE};
		auto pb{NetBridge::C2S::Make_CL_ADD_BOT_PACKET(static_cast<FB_ENUMS::TEAM_TYPE>(flag))};
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
		flag = (flag == FB_ENUMS::TEAM_TYPE_BLUE) ? FB_ENUMS::TEAM_TYPE_RED : FB_ENUMS::TEAM_TYPE_BLUE;
	}
	
	// 게임 시작하기
	if (GLOBAL(InputGlobal).GetInputDown('R'))
	{
		auto pb{NetBridge::C2S::Make_CL_START_GAME_PACKET()};
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}
#endif
}