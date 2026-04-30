#include "stdafxClient.h"
#include "LoginControllerComponent.h"
#include "InputGlobal.h"
#include "NetworkGlobal.h"
#include "Packets/C2SPackets.h"

void LoginControllerComponent::OnUpdate(float deltaTime)
{
	// R 키 입력
#ifdef APPLY_LOBBY_SERVER
	if (!m_isLoginRequested && GLOBAL(InputGlobal).GetInputDown('R'))
	{
		DEBUG_LOG_FMT("[LoginControllerComponent] 'R' Key Detected! Sending CS_LOGIN...\n");

		// 테스트용 ID/PW로 로그인 패킷 전송
		auto pb = NetBridge::C2S::Make_CL_LOGIN_PACKET("TestID", "TestPW");
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));

		m_isLoginRequested = true;
	}
#endif
}
