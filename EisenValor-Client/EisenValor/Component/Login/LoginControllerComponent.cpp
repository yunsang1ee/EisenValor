#include "stdafxClient.h"
#include "LoginControllerComponent.h"
#include "InputGlobal.h"
#include "NetworkGlobal.h"
#include "Packets/C2SPackets.h"

void LoginControllerComponent::OnUpdate(float deltaTime)
{
	// R 키 입력
#ifdef APPLY_LOBBY_SERVER
	if (GLOBAL(InputGlobal).GetInputDown('R'))
	{
		DEBUG_LOG_FMT("[LoginControllerComponent] 'R' Key Detected! Sending CS_LOGIN...\n");

		// 테스트용 ID/PW로 로그인 패킷 전송
		auto pb = NetBridge::C2S::Make_CL_LOGIN_PACKET("TestID", "TestPW");
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}

	if (GLOBAL(InputGlobal).GetInputDown('S'))
	{
		DEBUG_LOG_FMT("[LoginControllerComponent] 'S' Key Detected! Sending CL_SIGN_UP...\n");

		auto pb{NetBridge::C2S::Make_CL_SIGN_UP_PACKET("TestID", "TestPW", "TestNickName")};
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}

#endif
}
