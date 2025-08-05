#include "stdafxClientFramework.h"
#include "PacketHandler.h"
#include "NetworkManager.h"
#include "LocalPlayer.h"
#include "GameObjectManager.h"
#include "DxDeviceGlobal.h"

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
	const auto packetData = NetBridge::ServerPacketHandler::Make_CS_ENTER_MATCH_PACKET(id);
	auto	   packetBuffer = NetBridge::ServerPacketHandler::MakeSendBuffer(PACKET_TYPE::CS_ENTER_MATCH, packetData);
	MANAGER(NetBridge::NetworkManager)->Send(std::move(packetBuffer));

	return true;
}

bool Handle_SC_CHAT_PACKET(const SOCKET& socket, const FB_TABLES::SC_CHAT_PACKET& recvPkt)
{
	std::cout << recvPkt.msg()->c_str() << std::endl;
	return true;
}

bool Handle_SC_PLAYER_MOVE_PACKET(const SOCKET& socket, const FB_TABLES::SC_PLAYER_MOVE_PACKET& recvPkt)
{
	auto localPlayer = MANAGER(GameObjectManager)->GetLocalPlayer();
	if (localPlayer == nullptr)
		return false;

	const uint32 localID = localPlayer->m_id;
	const uint32 id = recvPkt.player_id();

	if (localID == id)
		return false;

	auto obj = MANAGER(GameObjectManager)->FindObject(id);
	if (obj)
	{
		const Vec3 pos{recvPkt.pos()->x(), recvPkt.pos()->y(), recvPkt.pos()->z()};
		const Vec3 rot{recvPkt.rot()->x(), recvPkt.rot()->y(), recvPkt.rot()->z()};
		obj->SetPosition(pos.x, pos.y, pos.z);
		obj->SetRotation(rot.y);
	}

	return true;
}

bool Handle_SC_ADD_PLAYER_INFO_PACKET(const SOCKET& socket, const FB_TABLES::SC_ADD_PLAYER_INFO_PACKET& recvPkt)
{
	const uint32 id = recvPkt.player_id();

	const uint32 localID = MANAGER(GameObjectManager)->GetLocalID();

	auto device = GlobalRegistry::Get<IDxDeviceGlobal>().GetDevice();

	if (id == localID)
	{
		auto localPlayer = std::make_shared<LocalPlayer>();
		localPlayer->Initialize(device);
		localPlayer->m_id = recvPkt.player_id();
		localPlayer->SetPosition(recvPkt.pos()->x(), recvPkt.pos()->y(), recvPkt.pos()->z());
		localPlayer->SetRotation(recvPkt.rot()->y());

		MANAGER(GameObjectManager)->SetLocalPlayer(localPlayer);
		MANAGER(GameObjectManager)->AddObject(localPlayer);
	}
	else
	{
		auto player = std::make_shared<Player>();
		player->Initialize(device);
		player->m_id = id;

		const Vec3 pos{recvPkt.pos()->x(), recvPkt.pos()->y(), recvPkt.pos()->z()};
		const Vec3 rot{recvPkt.rot()->x(), recvPkt.rot()->y(), recvPkt.rot()->z()};
		player->SetPosition(pos.x, pos.y, pos.z);
		player->SetRotation(rot.y);

		std::println("----------------------------------------------------");
		std::println("Hello! ADD_PLAYER, ID:{}", id);
		std::println("Pos X:{}, Y:{}, Z:{}, ", pos.x, pos.y, pos.z);
		std::println("Rot X:{}, Y:{}, Z:{}, ", rot.x, rot.y, rot.z);
		std::println("----------------------------------------------------\n");

		MANAGER(GameObjectManager)->AddObject(player);
	}

	return true;
}

bool Handle_SC_REMOVE_PLAYER_INFO(const SOCKET& socket, const FB_TABLES::SC_REMOVE_PLAYER_INFO_PACKET& recvPkt)
{
	const uint32 id = recvPkt.player_id();
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

bool Handle_SC_ENTER_MATCH_PACKET(const SOCKET& socket, const FB_TABLES::SC_ENTER_MATCH_PACKET& recvPkt)
{
	// TODO: Handle_SC_ENTER_MATCH_PACKET
	// TODO: 서버로부터 위치 받기.

	return true;
}
