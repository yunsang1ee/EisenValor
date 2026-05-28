#include "stdafxClient.h"
#include "LoginControllerComponent.h"
#include "InputGlobal.h"
#include "NetworkGlobal.h"
#include "Packets/C2SPackets.h"

void LoginControllerComponent::OnUpdate(float deltaTime)
{
	// L 키 입력
#ifdef APPLY_LOBBY_SERVER
	if (GLOBAL(InputGlobal).GetInputDown('L'))
	{
		DEBUG_LOG_FMT("[LoginControllerComponent] 'L' Key Detected! Sending CS_LOGIN...\n");

		// 테스트용 ID/PW로 로그인 패킷 전송
		const std::string id = "TestID" + std::to_string(GetCurrentProcessId());
		auto pb = NetBridge::C2S::Make_CL_LOGIN_PACKET(id, "TestPW");
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}

	if (GLOBAL(InputGlobal).GetInputDown('R'))
	{
		DEBUG_LOG_FMT("[LoginControllerComponent] 'R' Key Detected! Sending CL_SIGN_UP...\n");

		const std::string id = "TestID" + std::to_string(GetCurrentProcessId());
		const std::string nickname = "TestNickName" + std::to_string(GetCurrentProcessId());
		auto pb{NetBridge::C2S::Make_CL_SIGN_UP_PACKET(id, "TestPW", nickname)};
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}

#endif
}
