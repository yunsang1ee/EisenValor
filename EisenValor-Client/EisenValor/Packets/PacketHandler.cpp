#include "stdafxClientFramework.h"
#include "PacketHandler.h"
#include "NetworkManager.h"
#include "DxDeviceGlobal.h"
#include "SceneGlobal.h"

std::array<PacketHandlerFunc, std::numeric_limits<uint16>::max() + 1> PacketHandlerFuncs;

bool Handle_Invalid(const SOCKET&, const char* const, const PacketHeader&)
{
	return false;
}

bool Handle_SC_LOGIN_PACKET(const SOCKET& socket, const FB_TABLES::SC_LOGIN_PACKET& recvPkt)
{
	const uint32 id{recvPkt.player_id()};

	// std::println("Player ID: {}", id);
	auto device = MANAGER(DxDeviceGlobal).GetDevice();

	MANAGER(SceneGlobal).SetLocalNetworkID(id);
	// TODO: 들어갈 수 Room 목록 중, ROOM 선택해서 들어갈 수 있게끔..
	const uint16 roomID{1};

	auto pb = NetBridge::ServerPacketHandler::Make_CS_ENTER_ROOM_PACKET(id, roomID);
	MANAGER(NetBridge::NetworkManager).Send(std::move(pb));

	return true;
}

bool Handle_SC_ENTER_ROOM_PACKET(const SOCKET& socket, const FB_TABLES::SC_ENTER_ROOM_PACKET& recvPkt)
{
	return false;
}

bool Handle_SC_LOCAL_PLAYER_PACKET(const SOCKET& socket, const FB_TABLES::SC_LOCAL_PLAYER_PACKET& recvPkt)
{
	auto		 device = MANAGER(DxDeviceGlobal).GetDevice();
	auto&		 sceneGlobal = MANAGER(SceneGlobal);
	auto*		 scene = sceneGlobal.GetActiveScene();

	const uint32 id = recvPkt.player_id();
	const uint32 localID = sceneGlobal.GetLocalNetworkID();

	if (id == localID)
	{
		// TODO: 로컬
		auto localPlayer;
		localPlayer->SetTeam(static_cast<GameObject::Team>(recvPkt.team_type()));
		localPlayer->SetTeamColor();
		localPlayer->Initialize(device);

		// TODO: 팀 설정
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

		MANAGER(GameObjectManager).SetLocalPlayer(localPlayer);
		MANAGER(GameObjectManager).AddObject(localPlayer);
		return true;
	}

	return false;
}

bool Handle_SC_ADD_OBJ_PACKET(const SOCKET& socket, const FB_TABLES::SC_ADD_OBJ_PACKET& recvPkt)
{
	const uint32 id = recvPkt.obj_id();
	const uint32 localID = MANAGER(GameObjectManager).GetLocalID();

	if (id == localID)
		return false;

	auto device = MANAGER(DxDeviceGlobal).GetDevice();

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
		player->SetTeam(static_cast<GameObject::Team>(recvPkt.team_type()));
		player->SetTeamColor();
		player->Initialize(device);
		player->m_id = id;
		player->SetPosition(pos);
		player->SetRotation(rot);
		player->SetVelocity(vel);
		player->SetAccelration(accel);
		player->lastServerPosition = pos;

		MANAGER(GameObjectManager).AddObject(player);
		break;
	}
	case ObjectType::NPC:
	{
		auto npc = std::make_shared<NPC>();
		npc->SetTeam(static_cast<GameObject::Team>(recvPkt.team_type()));
		npc->SetTeamColor();
		npc->SetUnitType(static_cast<NPC::NPC_TYPE>(recvPkt.npc_type()));

		npc->Initialize(device);
		npc->m_id = id;
		npc->SetPosition(pos);
		npc->SetRotation(rot);
		npc->SetVelocity(vel);
		npc->SetAccelration(accel);
		npc->lastServerPosition = pos;
		MANAGER(GameObjectManager).AddObject(npc);
		break;
	}
	default:
		break;
	}

	return true;
}

bool Handle_SC_REMOVE_OBJ_PACKET(const SOCKET& socket, const FB_TABLES::SC_REMOVE_OBJ_PACKET& recvPkt)
{
	const uint32 id = recvPkt.obj_id();
	const uint32 localID = MANAGER(GameObjectManager).GetLocalID();

	if (id == localID)
	{
		return false;
	}
	else
	{
		// TODO: 오브젝트 삭제처리
		auto obj = MANAGER(GameObjectManager).FindObject(id);
		if (obj)
			obj->alive = false;
	}

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
	const uint32 localID = MANAGER(GameObjectManager).GetLocalID();

	if (id == localID)
	{
		// 만약, 나에게 온 정보라면 서버로부터 받은 좌표로 이동해야 함.
		return false;
	}

	auto obj = MANAGER(GameObjectManager).FindObject(id);
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

bool Handle_SC_HIT_PACKET(const SOCKET& socket, const FB_TABLES::SC_HIT_PACKET& recvPkt)
{
	const uint32 objID{recvPkt.obj_id()};

	auto obj = MANAGER(GameObjectManager).FindObject(objID);
	if (obj)
	{
		const uint32 hp{recvPkt.current_hp()};
		// TODO: 받은 HP 설정
		if (hp <= 0)
		{
			obj->alive = false;
		}
	}

	return true;
}

bool Handle_SC_REMANING_GAME_TIME_PACKET(const SOCKET& socket, const FB_TABLES::SC_REMAINING_GAME_TIME& recvPkt)
{
	const uint32_t remainingTime{recvPkt.remaining_time()};
	const uint32_t totalSeconds = remainingTime / 1000;
	const uint32_t minutes = totalSeconds / 60;
	const uint32_t seconds = totalSeconds % 60;
	std::cout << std::format("{:02d}M:{:02d}S", minutes, seconds) << std::endl;
	return true;
}
