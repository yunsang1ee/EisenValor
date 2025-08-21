#include "stdafxClientFramework.h"
#include "PacketHandler.h"
#include "NetworkManager.h"
#include "LocalPlayer.h"
#include "GameObjectManager.h"
#include "DxDeviceGlobal.h"
#include "NPC.h"

std::array<PacketHandlerFunc, std::numeric_limits<uint16>::max() + 1> PacketHandlerFuncs;

bool Handle_Invalid(const SOCKET&, const char* const, const PacketHeader&)
{
	return false;
}

bool Handle_SC_LOGIN_PACKET(const SOCKET& socket, const FB_TABLES::SC_LOGIN_PACKET& recvPkt)
{
	const uint32 id{recvPkt.player_id()};

	std::println("Player ID: {}", id);
	auto& device = GlobalRegistry::Get<IDxDeviceGlobal>();

	MANAGER(GameObjectManager)->SetLocalID(id);
	auto pb = NetBridge::ServerPacketHandler::Make_CS_ENTER_WORLD_PACKET(id);
	MANAGER(NetBridge::NetworkManager)->Send(std::move(pb));

	return true;
}

bool Handle_SC_ENTER_WORLD_PACKET(const SOCKET& socket, const FB_TABLES::SC_ENTER_WORLD_PACKET& recvPkt)
{
	return false;
}

bool Handle_SC_LOCAL_PLAYER_PACKET(const SOCKET& socket, const FB_TABLES::SC_LOCAL_PLAYER_PACKET& recvPkt)
{
	auto		 device = GlobalRegistry::Get<IDxDeviceGlobal>().GetDevice();
	const uint32 id = recvPkt.player_id();
	const uint32 localID = MANAGER(GameObjectManager)->GetLocalID();
	if (id == localID)
	{
		auto localPlayer = std::make_shared<LocalPlayer>();
		localPlayer->Initialize(device);
		localPlayer->m_id = recvPkt.player_id();

		const Vec3 pos{
			recvPkt.kinematic_info()->pos().x(), recvPkt.kinematic_info()->pos().y(),
			recvPkt.kinematic_info()->pos().z()
		};

		const Vec3 rot{
			recvPkt.kinematic_info()->rot().x(), recvPkt.kinematic_info()->rot().y(),
			recvPkt.kinematic_info()->rot().z()
		};

		localPlayer->SetPosition(pos);
		localPlayer->SetRotation(rot);

		MANAGER(GameObjectManager)->SetLocalPlayer(localPlayer);
		MANAGER(GameObjectManager)->AddObject(localPlayer);
		return true;
	}

	return false;
}

bool Handle_SC_ADD_OBJ_PACKET(const SOCKET& socket, const FB_TABLES::SC_ADD_OBJ_PACKET& recvPkt)
{
	const uint32 id = recvPkt.obj_id();
	const uint32 localID = MANAGER(GameObjectManager)->GetLocalID();

	if (id == localID)
		return false;

	auto device = GlobalRegistry::Get<IDxDeviceGlobal>().GetDevice();

	const Vec3 pos{
		recvPkt.kinematic_info()->pos().x(), recvPkt.kinematic_info()->pos().y(), recvPkt.kinematic_info()->pos().z()
	};
	const Vec3 rot{
		recvPkt.kinematic_info()->rot().x(), recvPkt.kinematic_info()->rot().y(), recvPkt.kinematic_info()->rot().z()
	};
	const Vec3 vel{
		recvPkt.kinematic_info()->vel().x(), recvPkt.kinematic_info()->vel().y(), recvPkt.kinematic_info()->vel().z()
	};
	
	const Vec3 accel{
		recvPkt.kinematic_info()->accel().x(), recvPkt.kinematic_info()->accel().y(),
		recvPkt.kinematic_info()->accel().z()
	};

	switch (auto type = static_cast<ObjectType>(recvPkt.obj_type()))
	{
	case ObjectType::PLAYER:
	{
		auto player = std::make_shared<Player>();
		
		player->Initialize(device);
		player->m_id = id;
		player->SetPosition(pos);
		player->SetRotation(rot);
		player->SetVelocity(vel);
		player->SetAccelration(accel);
		player->lastServerPosition = pos;

		MANAGER(GameObjectManager)->AddObject(player);
		break;
	}
	case ObjectType::NPC:
	{
		auto npc = std::make_shared<NPC>();
		npc->SetTeam(static_cast<NPC::Team>(recvPkt.team_type()));
		npc->SetUnitType(static_cast<NPC::NPC_TYPE>(recvPkt.npc_type()));

		npc->Initialize(device);
		npc->m_id = id;

		npc->SetPosition(pos);
		npc->SetRotation(rot);
		npc->SetVelocity(vel);
		npc->SetAccelration(accel);
		npc->lastServerPosition = pos;
		MANAGER(GameObjectManager)->AddObject(npc);
		break;
	}
	default:
		break;
	}

	//std::println("----------------------------------------------------");
	//std::println("Hello! ADD_OBJ, ID:{}", id);
	//std::println("Pos X:{}, Y:{}, Z:{}, ", pos.x, pos.y, pos.z);
	//std::println("Rot X:{}, Y:{}, Z:{}, ", rot.x, rot.y, rot.z);
	//std::println("----------------------------------------------------\n");

	return true;
}

bool Handle_SC_REMOVE_OBJ_PACKET(const SOCKET& socket, const FB_TABLES::SC_REMOVE_OBJ_PACKET& recvPkt)
{
	const uint32 id = recvPkt.obj_id();
	const uint32 localID = MANAGER(GameObjectManager)->GetLocalID();

	if (id == localID)
	{
		return false;
	}
	else
	{
		auto obj = MANAGER(GameObjectManager)->FindObject(id);
		if (obj)
			obj->alive = false;
	}

	std::println("----------------------------------------------------");
	std::println("BYE! REMOVE_PLAYER, ID:{}", id);
	std::println("----------------------------------------------------\n");

	return true;
}

bool Handle_SC_CHAT_PACKET(const SOCKET& socket, const FB_TABLES::SC_CHAT_PACKET& recvPkt)
{
	std::cout << recvPkt.msg()->c_str() << std::endl;
	return true;
}

bool Handle_SC_MOVE_PACKET(const SOCKET& socket, const FB_TABLES::SC_MOVE_PACKET& recvPkt)
{
	const uint32 id = recvPkt.obj_id();
	const uint32 localID = MANAGER(GameObjectManager)->GetLocalID();

	if (id == localID)
	{
		// 만약, 나에게 온 정보라면 서버로부터 받은 좌표로 이동해야 함.
		return false;
	}

	auto obj = MANAGER(GameObjectManager)->FindObject(id);
	if (obj)
	{
		const Vec3 pos{
			recvPkt.kinematic_info()->pos().x(), recvPkt.kinematic_info()->pos().y(),
			recvPkt.kinematic_info()->pos().z()
		};
		const Vec3 rot{
			recvPkt.kinematic_info()->rot().x(), recvPkt.kinematic_info()->rot().y(),
			recvPkt.kinematic_info()->rot().z()
		};
		const Vec3 vel{
			recvPkt.kinematic_info()->vel().x(), recvPkt.kinematic_info()->vel().y(),
			recvPkt.kinematic_info()->vel().z()
		};
		const Vec3 accel{
			recvPkt.kinematic_info()->accel().x(), recvPkt.kinematic_info()->accel().y(),
			recvPkt.kinematic_info()->accel().z()
		};
		obj->Handle_SC_MOVE(pos, rot, vel, accel, recvPkt.kinematic_info()->time_stamp());
	}
	return true;
}