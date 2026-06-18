#include "stdafxClient.h"
#include "LoginSceneControllerComponent.h"
#include "InputGlobal.h"
#include "NetworkGlobal.h"
#include "Packets/C2SPackets.h"

void LoginSceneControllerComponent::OnUpdate(float deltaTime)
{
	// L 키 입력
#ifdef APPLY_LOBBY_SERVER
	if (GLOBAL(InputGlobal).GetInputDown('L'))
	{
		DEBUG_LOG_FMT("[LoginControllerComponent] 'L' Key Detected! Sending CS_LOGIN...\n");

		if (m_id.empty())
		{
			std::cout << "Enter ID: ";
			std::getline(std::cin, m_id);
		}

		auto pb = NetBridge::C2S::Make_CL_LOGIN_PACKET(m_id, "TestPW");
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}

	if (GLOBAL(InputGlobal).GetInputDown('R'))
	{
		DEBUG_LOG_FMT("[LoginControllerComponent] 'R' Key Detected! Sending CL_SIGN_UP...\n");
		if (m_id.empty())
		{
			std::cout << "Enter ID: ";
			std::getline(std::cin, m_id);
		}
		const std::string nickname = "TestNickName" + std::to_string(GetCurrentProcessId());
		auto pb{NetBridge::C2S::Make_CL_SIGN_UP_PACKET(m_id, "TestPW", nickname)};
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}

#endif
}
