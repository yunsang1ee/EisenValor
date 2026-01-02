#include "stdafxClient.h"
#include "PacketHandler.h"
#include "NetworkGlobal.h"
#include "DxDeviceGlobal.h"
#include "SceneGlobal.h"
#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"
#include "MeshComponent.h"
#include "MovementComponent.h"

std::array<PacketHandlerFunc, std::numeric_limits<uint16>::max() + 1> PacketHandlerFuncs;

namespace Resources
{
//clang-format off
std::vector<Vertex> cubeVertices = {
	// Front face (z = 0.5)
	{{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
	{{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
	{{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
	{{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},

	// Back face (z = -0.5)
	{{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},
	{{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
	{{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.5f, 0.5f, 0.5f, 1.0f}},
	{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},

	// Left face (x = -0.5)
	{{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
	{{-0.5f, -0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.5f, 0.5f, 0.5f, 1.0f}},

	// Right face (x = 0.5)
	{{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
	{{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},
	{{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
	{{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},

	// Top face (y = 0.5)
	{{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},
	{{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
	{{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
	{{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 0.5f, 1.0f}},

	// Bottom face (y = -0.5)
	{{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
	{{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},
	{{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
	{{-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}}
};

std::vector<uint32_t> cubeIndices = {
	0,	1,	2,	0,	2,	3,	// Front
	4,	5,	6,	4,	6,	7,	// Back
	8,	9,	10, 8,	10, 11, // Left
	12, 13, 14, 12, 14, 15, // Right
	16, 17, 18, 16, 18, 19, // Top
	20, 21, 22, 20, 22, 23	// Bottom
};
// clang-format on
} // namespace Resources

bool Handle_Invalid(const SOCKET&, const char* const, const PacketHeader&)
{
	return false;
}

bool Handle_SC_LOGIN_PACKET(const SOCKET& socket, const FB_TABLES::SC_LOGIN_PACKET& recvPkt)
{
	const uint32 id{recvPkt.player_id()};

	DEBUG_LOG_FMT("[PacketHandler] Login Success. ID: {}\n", id);
	auto device = GLOBAL(DxDeviceGlobal).GetDevice();

	GLOBAL(SceneGlobal).SetLocalNetworkID(id);
	// TODO: Room 목록 중, ROOM 선택해서 들어갈 수 있게끔..
	const uint16 roomID{1};

	auto pb = NetBridge::ServerPacketHandler::Make_CS_ENTER_ROOM_PACKET(id, roomID);
	GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));

	return true;
}

bool Handle_SC_ENTER_ROOM_PACKET(const SOCKET& socket, const FB_TABLES::SC_ENTER_ROOM_PACKET& recvPkt)
{
	DEBUG_LOG_FMT("[PacketHandler] Entered Room: {}\n", recvPkt.room_id());
	return true;
}

bool Handle_SC_LOCAL_PLAYER_PACKET(const SOCKET& socket, const FB_TABLES::SC_LOCAL_PLAYER_PACKET& recvPkt)
{
	auto& sceneGlobal = GLOBAL(SceneGlobal);
	auto* scene = sceneGlobal.GetActiveScene();
	if (!scene)
	{
		return false;
	}

	const uint32 id = recvPkt.player_id();

	const auto&			kInfo = recvPkt.kinematic_info();
	std::optional<Vec3> pos =
		kInfo ? std::optional<Vec3>(Vec3(kInfo->pos().x(), kInfo->pos().y(), kInfo->pos().z())) : std::nullopt;
	std::optional<Vec3> rot =
		kInfo ? std::optional<Vec3>(Vec3(kInfo->rot().x(), kInfo->rot().y(), kInfo->rot().z())) : std::nullopt;

	scene->CreateGameObject(
		"LocalPlayer", id,
		[scene, pos, rot](GameObject* obj)
		{
			if (pos.has_value())
			{
				obj->GetTransform().SetPosition(pos.value());
			}
			if (rot.has_value())
			{
				obj->GetTransform().SetRotation(rot.value());
			}

			scene->CreateComponent<MovementComponent>(obj->GetHandle());

			scene->CreateComponentWithInit<MeshComponent>(
				obj->GetHandle(),
				[](MeshComponent* mesh) { mesh->SetMesh(Resources::cubeVertices, Resources::cubeIndices); }
			);

			scene->CreateComponentWithInit<CameraComponent>(
				obj->GetHandle(), [](CameraComponent* cam) { cam->SetAsMainCamera(); }
			);
		}
	);

	return true;
}

bool Handle_SC_ADD_OBJ_PACKET(const SOCKET& socket, const FB_TABLES::SC_ADD_OBJ_PACKET& recvPkt)
{
	auto& sceneGlobal = GLOBAL(SceneGlobal);
	auto* scene = sceneGlobal.GetActiveScene();
	if (!scene)
	{
		return false;
	}

	const uint32 id = recvPkt.obj_id();
	const uint32 localID = sceneGlobal.GetLocalNetworkID();

	if (id == localID)
	{
		return false;
	}

	const auto&			kInfo = recvPkt.kinematic_info();
	std::optional<Vec3> pos =
		kInfo ? std::optional<Vec3>(Vec3(kInfo->pos().x(), kInfo->pos().y(), kInfo->pos().z())) : std::nullopt;
	std::optional<Vec3> rot =
		kInfo ? std::optional<Vec3>(Vec3(kInfo->rot().x(), kInfo->rot().y(), kInfo->rot().z())) : std::nullopt;

	std::string name = "NetworkObject_" + std::to_string(id);

	scene->CreateGameObject(
		name, id,
		[scene, pos, rot](GameObject* obj)
		{
			if (pos.has_value())
			{
				obj->GetTransform().SetPosition(pos.value());
			}
			if (rot.has_value())
			{
				obj->GetTransform().SetRotation(rot.value());
			}

			scene->CreateComponent<MeshComponent>(obj->GetHandle());
		}
	);

	return true;
}

bool Handle_SC_REMOVE_OBJ_PACKET(const SOCKET& socket, const FB_TABLES::SC_REMOVE_OBJ_PACKET& recvPkt)
{
	auto& sceneGlobal = GLOBAL(SceneGlobal);
	auto* scene = sceneGlobal.GetActiveScene();
	if (!scene)
	{
		return false;
	}

	const uint32 id = recvPkt.obj_id();
	// Destruction is also deferred by Scene::DestroyGameObject
	auto* obj = scene->FindGameObjectByServerID(id);
	if (obj)
	{
		scene->DestroyGameObject(obj->GetHandle());
	}

	return true;
}

bool Handle_SC_CHAT_PACKET(const SOCKET& socket, const FB_TABLES::SC_CHAT_PACKET& recvPkt)
{
	if (recvPkt.msg())
		std::cout << "[CHAT] " << recvPkt.msg()->c_str() << std::endl;
	return true;
}

bool Handle_SC_MOVE_PACKET(const SOCKET& socket, const FB_TABLES::SC_MOVE_PACKET& recvPkt)
{
	auto& sceneGlobal = GLOBAL(SceneGlobal);
	auto* scene = sceneGlobal.GetActiveScene();
	if (!scene)
	{
		return false;
	}

	const uint32 id = recvPkt.obj_id();
	auto*		 obj = scene->FindGameObjectByServerID(id);

	if (obj)
	{
		const auto& kInfo = recvPkt.kinematic_info();
		if (kInfo)
		{
			obj->GetTransform().SetPosition(kInfo->pos().x(), kInfo->pos().y(), kInfo->pos().z());
			obj->GetTransform().SetRotation(kInfo->rot().x(), kInfo->rot().y(), kInfo->rot().z());
		}
	}
	return true;
}

bool Handle_SC_HIT_PACKET(const SOCKET& socket, const FB_TABLES::SC_HIT_PACKET& recvPkt)
{
	// Hit processing
	return true;
}

bool Handle_SC_REMANING_GAME_TIME_PACKET(const SOCKET& socket, const FB_TABLES::SC_REMAINING_GAME_TIME& recvPkt)
{
	const uint32_t remainingTime{recvPkt.remaining_time()};
	const uint32_t totalSeconds = remainingTime / 1000;
	const uint32_t minutes = totalSeconds / 60;
	const uint32_t seconds = totalSeconds % 60;
	// std::cout << std::format("{:02d}M:{:02d}S", minutes, seconds) << std::endl;
	return true;
}
