#include "stdafxClient.h"
#include "WorldSceneControllerComponent.h"
#include "InputGlobal.h"
#include "NetworkGlobal.h"
#include "SceneGlobal.h"
#include "Packets/C2SPackets.h"

void WorldSceneControllerComponent::OnUpdate(float deltaTime)
{
#ifdef APPLY_LOBBY_SERVER
	if (GLOBAL(InputGlobal).GetInputDown('L'))
	{
		GLOBAL(NetBridge::NetworkGlobal).DisconnectGameServer();
		GLOBAL(NetBridge::NetworkGlobal).ReconnectLobbyServer();
		auto pb{NetBridge::C2S::Make_CL_RETURN_TO_GAME_ROOM_PACKET(GLOBAL(SceneGlobal).GetSessionID())};
		GLOBAL(NetBridge::NetworkGlobal).SendLobby(std::move(pb));
	}
#endif
}
