#include "stdafxClient.h"
#include "WorldSceneControllerComponent.h"
#include "InputGlobal.h"
#include "NetworkGlobal.h"
#include "Packets/C2SPackets.h"

void WorldSceneControllerComponent::OnUpdate(float deltaTime)
{
	if (GLOBAL(InputGlobal).GetInputDown('L'))
	{
		auto pb{NetBridge::C2S::Make_CL_RETURN_TO_GAME_ROOM_PACKET()};
		GLOBAL(NetBridge::NetworkGlobal).SendLobby(std::move(pb));
	}
}