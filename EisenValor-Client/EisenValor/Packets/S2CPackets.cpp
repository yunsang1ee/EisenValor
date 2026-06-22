#include "stdafxClient.h"
#include "S2CPackets.h"
#include "C2SPackets.h"
#include "DxDeviceGlobal.h"
#include "SceneGlobal.h"
#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"
#include "Util/AnimationLoader.h"

// Resource
#include "ResourceGlobal.h"
#include "AudioGlobal.h"
#include "MeshComponent.h"
#include "MeshResource.h"
#include "SkinnedMeshComponent.h"
#include "SkinnedMeshResource.h"
#include "MaterialResource.h"

#include "CameraComponent.h"
#include "MovementComponent.h"
#include "DxSwapChain.h"
#include "DxRendererGlobal.h"
#include "Component/PlayerControllerComponent.h"
#include "Component/HealthComponent.h"
#include "Component/BattleUIControllerComponent.h"
#include "Component/World/QuestProgressComponent.h"
#include "Util/CameraConfig.h"
#include "Component/TeamComponent.h"
#include "Component/VitalUIControllerComponent.h"
#include "Component/StaminaComponent.h"
#include "Component/FSM/FSMComponent.h"
#include "Component/FSM/StateImplementations.h"
#include "RectTransformComponent.h"
#include "ImageUIComponent.h"
#include "ButtonUIComponent.h"
#include "TextUIComponent.h"
#include "Component/SocketComponent.h"
#include "Component/AttackRangeDebugComponent.h"
#include "Component/FootIKComponent.h"
#include "Scene/ScoreScene.h"
#include "ResourceGlobal.h"
#include "MeshResource.h"
#include <unordered_map>

using namespace NetBridge;

namespace
{
	std::unordered_map<uint64, uint8_t> s_pendingStateByObjectID;
	uint8 s_latestRedScore = 0;
	uint8 s_latestBlueScore = 0;

	void NotifyOccupationZoneReached(Scene* scene)
	{
		if (!scene)
		{
			return;
		}

		auto* localPlayer = scene->FindGameObjectByServerID(scene->GetLocalID());
		if (!localPlayer)
		{
			return;
		}

		if (auto* quest = localPlayer->GetComponent<QuestProgressComponent>())
		{
			quest->SetOccupationZoneReached(true);
		}
	}

	void ApplyPendingServerState(uint64 objID, FSMComponent* fsm)
	{
		if (!fsm) return;

		auto iter = s_pendingStateByObjectID.find(objID);
		if (iter == s_pendingStateByObjectID.end()) return;

		fsm->SetServerState(iter->second);
		s_pendingStateByObjectID.erase(iter);
	}

	TextUIComponent* FindRemainingTimeText(Scene* scene)
	{
		if (!scene)
		{
			return nullptr;
		}

		auto* textStorage = scene->GetStorage<TextUIComponent>();
		if (!textStorage)
		{
			return nullptr;
		}

		for (auto& text : textStorage->GetList())
		{
			auto* owner = text.GetGameObject();
			if (!owner)
			{
				continue;
			}

			if (owner->GetName() == "RemainingTimeText")
			{
				return &text;
			}
		}

		return nullptr;
	}
}

bool NetBridge::S2C::Handle_Invalid(const SOCKET&, const char* const, const PacketHeader&)
{
	return false;
}

#pragma region LOGIN_PACKETS
bool NetBridge::S2C::Handle_LC_LOGIN_FAIL_PACKET(const SOCKET& socket, const FB_TABLES::LC_LOGIN_FAIL_PACKET& recvPkt)
{
	// TODO: 로그인 실패 알림
	DEBUG_LOG_FMT("[SC_LOGIN_FAIL_PACKET] ");
	DEBUG_LOG_FMT("Fail Reason: {}\n", recvPkt.fail_msg()->c_str());
	return true;
}

bool NetBridge::S2C::Handle_LC_LOGIN_SUCCESS_PACKET(
	const SOCKET& socket, const FB_TABLES::LC_LOGIN_SUCCESS_PACKET& recvPkt
)
{
	// 여기서 ID는 Lobby Session ID
	const uint32 id = recvPkt.lobby_session_id();
	auto		 device = GLOBAL(DxDeviceGlobal).GetDevice();

	DEBUG_LOG_FMT("[SC_LOGIN_SUCCESS_PACKET] Login Success! Lobby Session ID: {}\n", id);
	const auto& nickName{recvPkt.nickname()->c_str()};
	DEBUG_LOG_FMT("[SC_LOGIN_SUCCESS_PACKET] Nickname: {}\n", nickName);

#ifdef APPLY_LOBBY_SERVER
	GLOBAL(SceneGlobal).SetSessionID(id);
	auto pb = NetBridge::C2S::Make_CL_ENTER_GAME_LOBBY_PACKET();
	GLOBAL(NetworkGlobal).Send(std::move(pb));
	DEBUG_LOG_FMT("[SC_LOGIN_SUCCESS_PACKET] id: {}, Scene changed to LobbyScene\n", id);
#endif

#ifndef APPLY_LOBBY_SERVER
	const uint16 roomID{1};
	auto		 pb = C2S::Make_CS_ENTER_GAME_WORLD_PACKET(roomID, id);
	GLOBAL(NetworkGlobal).Send(std::move(pb));
	DEBUG_LOG_FMT("[SC_LOGIN_SUCCESS_PACKET] id: {}, Scene changed to WorldScene\n", id);

#endif // !APPLY_LOBBY_SERVER

	NotifyOccupationZoneReached(GLOBAL(SceneGlobal).GetActiveScene());
	return true;
}
bool NetBridge::S2C::Handle_LC_SIGN_UP_FAIL_PACKET(
	const SOCKET& socket, const FB_TABLES::LC_SIGN_UP_FAIL_PACKET& recvPkt
)
{
	DEBUG_LOG_FMT("[SC_SIGN_UP_FAIL_PACKET] ");
	DEBUG_LOG_FMT("Fail Reason: {}\n", recvPkt.fail_msg()->c_str());

	return true;
}
bool NetBridge::S2C::Handle_LC_SIGN_UP_SUCCESS_PACKET(
	const SOCKET& socket, const FB_TABLES::LC_SIGN_UP_SUCCESS_PACKET& recvPkt
)
{
	DEBUG_LOG_FMT("[SC_SIGN_UP_SUCCESS_PACKET] Sign Up Success! ");
	return true;
}
#pragma endregion

#pragma region LOBBY_PACKETS
bool NetBridge::S2C::Handle_LC_ENTER_GAME_LOBBY_FAIL_PACKET(
	const SOCKET& socket, const FB_TABLES::LC_ENTER_GAME_LOBBY_FAIL_PACKET& recvPkt
)
{
	DEBUG_LOG_FMT("[SC_ENTER_GAME_LOBBY_FAIL_PACKET] ");
	DEBUG_LOG_FMT("Fail Reason: {}\n", recvPkt.fail_msg()->c_str());

	return true;
}
bool NetBridge::S2C::Handle_LC_ENTER_GAME_LOBBY_SUCCESS_PACKET(
	const SOCKET& socket, const FB_TABLES::LC_ENTER_GAME_LOBBY_SUCCESS_PACKET& recvPkt
)
{
	std::cout << "[SC_ENTER_GAME_LOBBY_PACKET] " << std::endl;

	const auto& rooms = recvPkt.rooms();
	const auto& users = recvPkt.users();
	const auto& vecUserID = recvPkt.vec_user_id();

	if (rooms == nullptr)
	{
		return true;
	}

	// 로비에 성공적으로 입장
	// TODO: 로비 씬 안에서 방 목록 및 유저 목록을 화면에 보여주기

	for (const auto* room : *rooms)
	{
		auto roomId = room->id();
		DEBUG_LOG_FMT("Room ID: {}\n", roomId);
	}

	if (rooms->size() == 0)
		std::cout << "Zero Room" << std::endl;

	for (flatbuffers::uoffset_t i = 0; i < users->size(); ++i)
	{
		const auto* user = users->Get(i);
		DEBUG_LOG_FMT("-Name:{}, ID:{}\n", user->c_str(), vecUserID->Get(i));
	}

	// auto pb = NetBridge::C2S::Make_CS_JOIN_GAME_ROOM_PACKET(1000);
	// GLOBAL(NetBridge::NetworkManager)->Send(std::move(pb));/

	GLOBAL(SceneGlobal).LoadScene("LobbyScene");

	return true;
}

bool NetBridge::S2C::Handle_LC_LEAVE_GAME_LOBBY_PACKET(
	const SOCKET& socket, const FB_TABLES::LC_LEAVE_GAME_LOBBY_PACKET& recvPkt
)
{
	DEBUG_LOG_FMT("[SC_LEAVE_GAME_LOBBY_PACKET] ");
	DEBUG_LOG_FMT("Leave Lobby!\n");
	// TODO: 로그인 Scene으로 다시...
	GLOBAL(SceneGlobal).LoadScene("LoginScene");
	return true;
}

bool NetBridge::S2C::Handle_LC_ENTER_USER_IN_GAME_LOBBY_PACKET(
	const SOCKET& socket, const FB_TABLES::LC_ENTER_USER_IN_GAME_LOBBY_PACKET& recvPkt
)
{
	// 로비에 새로운 유저가 입장
	// TODO: 로비에 새로운 유저가 입장했음을 화면에 보여주기

	DEBUG_LOG_FMT("[SC_ENTER_USER_IN_GAME_LOBBY_PACKET] ");
	DEBUG_LOG_FMT("User Name: {}\n", recvPkt.user_name()->c_str());

	return true;
}


bool NetBridge::S2C::Handle_LC_LEAVE_USER_IN_GAME_LOBBY_PACKET(
	const SOCKET& socket, const FB_TABLES::LC_LEAVE_USER_IN_GAME_LOBBY_PACKET& recvPkt
)
{
	// 로비에서 유저가 퇴장
	// TODO: 로비에서 유저가 퇴장했음을 화면에 보여주기
	DEBUG_LOG_FMT("[SC_REMOVE_USER_IN_GAME_LOBBY_PACKET] ");
	DEBUG_LOG_FMT("Remove User In Lobby! id = {}\n", recvPkt.user_id());
	return false;
}

bool NetBridge::S2C::Handle_LC_MAKE_GAME_ROOM_PACKET(
	const SOCKET& socket, const FB_TABLES::LC_MAKE_GAME_ROOM_PACKET& recvPkt
)
{
	// 게임 룸이 성공적으로 생성됨
	// TODO: 방이 생성되었음을 화면에 보여주기
	DEBUG_LOG_FMT("[SC_MAKE_GAME_ROOM_PACKET] ");
	const auto& roomInfo = recvPkt.room_info();
	DEBUG_LOG_FMT("Room ID: {}\n", roomInfo->id());
	return true;
}
#pragma endregion

#pragma region ROOM_PACKETS
bool NetBridge::S2C::Handle_LC_ENTER_GAME_ROOM_FAIL_PACKET(
	const SOCKET& socket, const FB_TABLES::LC_ENTER_GAME_ROOM_FAIL_PACKET& recvPkt
)
{
	// 게임 룸 입장 실패
	// TODO: 게임 룸 입장 실패 메시지 화면에 보여주기
	DEBUG_LOG_FMT("[SC_JOIN_GAME_ROOM_FAIL_PACKET] ");
	DEBUG_LOG_FMT("Fail Reason: {}\n", recvPkt.fail_msg()->c_str());
	return true;
}

bool NetBridge::S2C::Handle_LC_ENTER_GAME_ROOM_SUCCESS_PACKET(
	const SOCKET& socket, const FB_TABLES::LC_ENTER_GAME_ROOM_SUCCESS_PACKET& recvPkt
)
{
	// 게임 룸 입장 성공
	// TODO: 게임 룸 입장 성공 메시지 화면에 보여주기
	// TODO: 룸 내부 Scene으로 전환
	// TODO: 룸 내부의 참가자 목록도 및 참여자 정보도 같이 보여주기

	DEBUG_LOG_FMT("[SC_JOIN_GAME_ROOM_SUCCESS_PACKET] ");
	auto user = recvPkt.user();
	// GLOBAL(SceneGlobal).SetLocalNetworkID(user->id());
	GLOBAL(SceneGlobal).SetSessionID(user->id());

	DEBUG_LOG_FMT("User ID:{}\n", user->id());

	if (user->type() == FB_ENUMS::PARTICIPANT_TYPE_USER)
	{
		DEBUG_LOG_FMT("User Type: USER\n");
	}
	else if (user->type() == FB_ENUMS::PARTICIPANT_TYPE_HOST)
	{
		DEBUG_LOG_FMT("User Type: HOST\n");
	}

	else if (user->type() == FB_ENUMS::PARTICIPANT_TYPE_BOT)
	{
		DEBUG_LOG_FMT("User Type: BOT\n");
	}

	if (user->state_type() == FB_ENUMS::PARTICIPANT_STATE_TYPE_NOT_READY)
	{
		DEBUG_LOG_FMT("User State: NOT_READY\n");
	}
	else
		DEBUG_LOG_FMT("User State: READY\n");

	if (user->team_type() == FB_ENUMS::TEAM_TYPE_BLUE)
	{
		DEBUG_LOG_FMT("User TeamType: BLUE\n");
	}
	else
		DEBUG_LOG_FMT("User TeamType: DEFENSE\n");

	auto participants = recvPkt.participants();
	DEBUG_LOG_FMT("Current Participant Count: {}\n", participants->size());

	for (const auto participant : *participants)
	{
		DEBUG_LOG_FMT("Participant ID:{}\n", participant->id());
		if (participant->type() == FB_ENUMS::PARTICIPANT_TYPE_USER)
		{
			DEBUG_LOG_FMT("Participant Type: USER\n");
		}
		else if (participant->type() == FB_ENUMS::PARTICIPANT_TYPE_HOST)
		{
			DEBUG_LOG_FMT("Participant Type: HOST\n");
		}
		else if (participant->type() == FB_ENUMS::PARTICIPANT_TYPE_BOT)
		{
			DEBUG_LOG_FMT("Participant Type: BOT\n");
		}
		if (participant->state_type() == FB_ENUMS::PARTICIPANT_STATE_TYPE_NOT_READY)
		{
			DEBUG_LOG_FMT("Participant State: NOT_READY\n");
		}
		else
			DEBUG_LOG_FMT("Participant State: READY\n");
		if (participant->team_type() == FB_ENUMS::TEAM_TYPE_BLUE)
		{
			DEBUG_LOG_FMT("Participant TeamType: BLUE\n");
		}
		else
			DEBUG_LOG_FMT("Participant TeamType: RED\n");
	}

	GLOBAL(SceneGlobal).LoadScene("RoomScene");
	return true;
}

bool NetBridge::S2C::Handle_LC_LEAVE_GAME_ROOM_PACKET(
	const SOCKET& socket, const FB_TABLES::LC_LEAVE_GAME_ROOM_PACKET& recvPkt
)
{
	// 게임 룸 퇴장 성공

	DEBUG_LOG_FMT("[SC_LEAVE_GAME_ROOM_PACKET] ");
	DEBUG_LOG_FMT("Leave Game Room!\n");

	auto pb = NetBridge::C2S::Make_CL_ENTER_GAME_LOBBY_PACKET();
	GLOBAL(NetworkGlobal).Send(std::move(pb));

	return true;
}

bool NetBridge::S2C::Handle_LC_JOIN_PARTICIPANT_IN_GAME_ROOM_PACKET(
	const SOCKET& socket, const FB_TABLES::LC_JOIN_PARTICIPANT_IN_GAME_ROOM_PACKET& recvPkt
)
{
	// 게임 룸에 새로운 참가자가 입장
	// TODO: 게임 룸에 새로운 참가자가 입장했음을 화면에 보여주기
	DEBUG_LOG_FMT("[SC_JOIN_PARTICIPANT_IN_GAME_ROOM_PACKET] ");
	auto participant = recvPkt.participant();
	DEBUG_LOG_FMT("Participant ID:{}\n", participant->id());

	if (participant->type() == FB_ENUMS::PARTICIPANT_TYPE_USER)
	{
		DEBUG_LOG_FMT("Participant Type: USER\n");
	}
	else if (participant->type() == FB_ENUMS::PARTICIPANT_TYPE_HOST)
	{
		DEBUG_LOG_FMT("Participant Type: HOST\n");
	}
	else if (participant->type() == FB_ENUMS::PARTICIPANT_TYPE_BOT)
	{
		DEBUG_LOG_FMT("Participant Type: BOT\n");
	}
	if (participant->state_type() == FB_ENUMS::PARTICIPANT_STATE_TYPE_NOT_READY)
	{
		DEBUG_LOG_FMT("Participant State: NOT_READY\n");
	}
	else
		DEBUG_LOG_FMT("Participant State: READY\n");

	if (participant->team_type() == FB_ENUMS::TEAM_TYPE_BLUE)
	{
		DEBUG_LOG_FMT("Participant TeamType: BLUE\n");
	}
	else
		DEBUG_LOG_FMT("Participant TeamType: RED\n");


	return true;
}

bool NetBridge::S2C::Handle_LC_LEAVE_PARTICIPANT_IN_GAME_ROOM_PACKET(
	const SOCKET& socket, const FB_TABLES::LC_LEAVE_PARTICIPANT_IN_GAME_ROOM_PACKET& recvPkt
)
{
	// 게임 룸에서 참가자가 퇴장
	DEBUG_LOG_FMT("[SC_LEAVE_PARTICIPANT_IN_GAME_ROOM_PACKET] ");
	DEBUG_LOG_FMT("Participant ID:{} Leave Game Room!\n", recvPkt.participant_id());
	return true;
}

bool NetBridge::S2C::Handle_LC_READY_GAME_PACKET(const SOCKET& socket, const FB_TABLES::LC_READY_GAME_PACKET& recvPkt)
{
	// 참가자가 준비 상태로 변경됨
	// TODO: 참가자가 준비 상태로 변경되었음을 화면에 보여주기
	DEBUG_LOG_FMT("User ID:{} ", recvPkt.user_id());

	if (recvPkt.participant_state() == FB_ENUMS::PARTICIPANT_STATE_TYPE_NOT_READY)
	{
		DEBUG_LOG_FMT(" Not Ready!\n");
	}
	else
		DEBUG_LOG_FMT(" Ready!\n");

	return true;
}

bool NetBridge::S2C::Handle_LC_START_GAME_FAIL_PACKET(
	const SOCKET& socket, const FB_TABLES::LC_START_GAME_FAIL_PACKET& recvPkt
)
{
	// 게임 시작 실패
	DEBUG_LOG_FMT("[LC_START_GAME_FAIL_PACKET] ");
	DEBUG_LOG_FMT("Fail Reason: {}\n", recvPkt.fail_msg()->c_str());

	return true;
}

bool NetBridge::S2C::Handle_LC_CHANGE_TEAM_PACKET(const SOCKET& socket, const FB_TABLES::LC_CHANGE_TEAM_PACKET& recvPkt)
{
	// 참가자가 팀을 변경함
	// TODO: 참가자가 팀을 변경했음을 화면에 보여주기
	if (recvPkt.team_type() == FB_ENUMS::TEAM_TYPE_BLUE)
	{
		DEBUG_LOG_FMT("User ID: {} Change Team To BLUE\n", recvPkt.user_id());
	}
	else
	{
		DEBUG_LOG_FMT("User ID: {} Change Team To RED\n", recvPkt.user_id());
	}
	return true;
}

bool NetBridge::S2C::Handle_LC_ADD_BOT_PACKET(const SOCKET& socket, const FB_TABLES::LC_ADD_BOT_PACKET& recvPkt)
{
	// 게임 룸에 새로운 봇이 추가됨
	DEBUG_LOG_FMT("[LC_ADD_BOT_PACKET] ");
	// DEBUG_LOG_FMT("Bot ID: {}, TeamType: {}\n", recvPkt.bot_id(), recvPkt.team_type());

	return true;
}

bool NetBridge::S2C::Handle_LC_REMOVE_BOT_PACKET(const SOCKET& socket, const FB_TABLES::LC_REMOVE_BOT_PACKET& recvPkt)
{
	// 게임 룸에서 봇이 제거됨
	DEBUG_LOG_FMT("[LC_REMOVE_BOT_PACKET] ");
	// DEBUG_LOG_FMT("Bot ID: {} Remove From Game Room!\n", recvPkt.bot_id());
	return true;
}

bool NetBridge::S2C::Handle_LC_CONNECT_TO_GAME_SERVER_PACKET(
	const SOCKET& socket, const FB_TABLES::LC_CONNECT_TO_GAME_SERVER_PACKET& recvPkt
)
{
	DEBUG_LOG_FMT("Handle_LC_CON	NECT_TO_GAME_SERVER");
	const uint32 sessionID = GLOBAL(SceneGlobal).GetSessionID();
	const uint16 worldID{recvPkt.world_id()};
	std::string ip{recvPkt.ip()->c_str()};
	const uint16 port{recvPkt.port()};

	if (false == GLOBAL(NetworkGlobal).ConnectGameServer(ip.c_str(), port))
		assert(nullptr);

	// 로비서버로부터 받은 세션 아이디
	// TODO: 로비 서버로부터 받은 세션 아이디를 게임 서버로 전달해서 게임 서버에 입장하기
	{
		auto pb{C2S::Make_CS_ENTER_GAME_WORLD_PACKET(worldID, sessionID)};
		GLOBAL(NetworkGlobal).SendGame(std::move(pb));
	}

	GLOBAL(NetworkGlobal).DisconnectLobbyServer();
	return true;
}

bool NetBridge::S2C::Handle_LC_RETURN_TO_GAME_ROOM_PACKET(
	const SOCKET& socket, const FB_TABLES::LC_RETURN_TO_GAME_ROOM_PACKET& recvPkt
)
{
	GLOBAL(NetworkGlobal).DisconnectGameServer();
	GLOBAL(SceneGlobal).LoadScene("RoomScene");
	return true;
}

bool NetBridge::S2C::Handle_LC_CHAT_PACKET(const SOCKET& socket, const FB_TABLES::LC_CHAT_PACKET& recvPkt)
{
	DEBUG_LOG_FMT("[LC_CHAT_PACKET] ");
	DEBUG_LOG_FMT("User ID: {}, Message: {}\n", recvPkt.session_id(), recvPkt.msg()->c_str());
	return true;
}
bool NetBridge::S2C::Handle_LC_GAME_RESULT_PACKET(const SOCKET& socket, const FB_TABLES::LC_GAME_RESULT_PACKET& recvPkt)
{
	// TODO: UI로 게임결과 보여주거나 게임 결과 씬으로 전환하거나

	DEBUG_LOG_FMT("[LC_GAME_RESULT_PACKET] ");

	const auto winningTeam{recvPkt.winning_team()};
	const auto blueScore{recvPkt.blue_score()};
	const auto redScore{recvPkt.red_score()};

	switch (winningTeam)
	{
	case FB_ENUMS::TEAM_TYPE_NONE:
		// 무승부
		break;
	case FB_ENUMS::TEAM_TYPE_BLUE:
		// 블루팀 승리
		break;
	case FB_ENUMS::TEAM_TYPE_RED:
		// 레드팀 승리
		break;
	default:
		break;
	}

	auto pb{NetBridge::C2S::Make_CL_RETURN_TO_GAME_ROOM_PACKET(GLOBAL(SceneGlobal).GetSessionID())};
	GLOBAL(NetBridge::NetworkGlobal).SendLobby(std::move(pb));

	return true;
}
#pragma endregion

bool NetBridge::S2C::Handle_SC_LOCAL_PLAYER_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_LOCAL_PLAYER_PACKET& recvPkt
)
{
	s_latestRedScore = 0;
	s_latestBlueScore = 0;

	// GLOBAL(SceneGlobal).SetLocalNetworkID(recvPkt.player_id());

	GLOBAL(SceneGlobal).SetLocalGameObjectID(recvPkt.player_id());
	GLOBAL(SceneGlobal).LoadScene("WorldScene");
	DEBUG_LOG_FMT("[SC_LOCAL_PLAYER_PACKET] \n");

	auto device = GLOBAL(DxDeviceGlobal).GetDevice();
	auto scene = GLOBAL(SceneGlobal).GetActiveScene();

	const uint64 id = recvPkt.player_id();
	const uint64 localID = scene->GetLocalID();
	if (id != localID)
	{
		DEBUG_LOG_FMT("is not LocalPlayer, id:{}, LocalID: {}\n", id, localID);
		return false;
	}
	DEBUG_LOG_FMT("Created LocalPlayer: {} \n", id);
	const Vec3 pos{recvPkt.pos_info()->pos().x(), recvPkt.pos_info()->pos().y(), recvPkt.pos_info()->pos().z()};
	const Vec3 rot{recvPkt.pos_info()->rot().x(), recvPkt.pos_info()->rot().y(), recvPkt.pos_info()->rot().z()};

	auto playerObjHandle = scene->ReserveGameObject(
		"LocalPlayer", id,
		[pos, rot, scene, stance = recvPkt.stance_type(), teamType = recvPkt.team_type(), maxHP = recvPkt.max_hp(),
		 currentHP = recvPkt.current_hp(), maxStamina = recvPkt.max_stamina(),
		 currentStamina = recvPkt.current_stamina()](GameObject* playerObj)
		{
			auto& tr = playerObj->GetTransform();
			tr.SetWorldPosition(DX::XMFLOAT3(pos.x, pos.y, pos.z));
			tr.SetRotation(rot.x, rot.y, rot.z);

			auto playerObjHandle = playerObj->GetHandle();

			scene->CreateComponentWithInit<SkinnedMeshComponent>(

				playerObjHandle,
				[](SkinnedMeshComponent* mesh)
				{
					auto meshRes =
						GLOBAL(ResourceGlobal).Load<SkinnedMeshResource>("Resource/Models/Cursed_Knight.evskin");
					if (nullptr != meshRes)
					{
						mesh->SetSkinnedMeshResource(meshRes, true);
					}
				}
			);

			// Add Equipment (Belts, Armors, Scarf)
			auto addEquipment = [scene, playerObjHandle](const std::string& name, const std::string& resPath)
			{
				auto handle = scene->ReserveGameObject("LocalPlayer_" + name);
				scene->CreateComponentWithInit<SkinnedMeshComponent>(
					handle,
					[scene, playerObjHandle, resPath](SkinnedMeshComponent* mesh)
					{
						auto res = GLOBAL(ResourceGlobal).Load<SkinnedMeshResource>(resPath);
						if (res)
						{
							mesh->SetSkinnedMeshResource(res);
						}

						if (auto* player = scene->TryGetGameObject(playerObjHandle))
						{
							auto* obj = mesh->GetGameObject();
							obj->GetTransform().SetParent(player->GetTransform().GetHandle());
							obj->GetTransform().SetPosition(0, 0, 0);
							obj->GetTransform().SetRotation(0, 0, 0);
						}
					}
				);
			};

			addEquipment("Belts", "Resource/Models/Belts.evskin");
			addEquipment("PrimaryArmor", "Resource/Models/Primary_Armors.evskin");
			addEquipment("SecondaryArmor", "Resource/Models/Secondary_Armors.evskin");
			addEquipment("LegsArmor", "Resource/Models/Leg_Armors.evskin");
			// addEquipment("Scarf", "Resource/Models/Scarf.evskin");
			addEquipment("Dress", "Resource/Models/Dress.evskin");

			scene->CreateComponentWithInit<MovementComponent>(
				playerObjHandle,
				[](MovementComponent* move)
				{
					move->SetMovementMode(MovementMode::Physics);
					move->SetMoveSpeed(5.0f);
				}
			);

			// BattleUIControllerComponent
			scene->CreateComponentWithInit<BattleUIControllerComponent>(
				playerObjHandle,
				[stance](BattleUIControllerComponent* ui)
				{
					ui->SetControlMode(BattleUIControllerComponent::ControlType::Local);
					ui->InitStance(stance);
				}
			);

			// VitalUI (HP/Stamina/Team)
			scene->CreateComponentWithInit<HealthComponent>(
				playerObjHandle,
				[maxHP, currentHP](HealthComponent* health)
				{
					health->SetMaxHealth(maxHP);
					health->SetHealth(currentHP);
				}
			);

			scene->CreateComponentWithInit<StaminaComponent>(
				playerObjHandle,
				[maxStamina, currentStamina](StaminaComponent* stamina)
				{
					stamina->SetMaxStamina(maxStamina);
					stamina->SetStamina(currentStamina);
				}
			);

			scene->CreateComponentWithInit<TeamComponent>(
				playerObjHandle, [teamType](TeamComponent* team) { team->SetTeamType(teamType); }
			);

			scene->CreateComponentWithInit<VitalUIControllerComponent>(
				playerObjHandle, [](VitalUIControllerComponent* vital) {}
			);

			// FSMComponent
			scene->CreateComponentWithInit<FSMComponent>(
				playerObjHandle,
				[](FSMComponent* fsm)
				{
					fsm->SetObjectType(static_cast<uint8_t>(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER));
					fsm->RequestState(FSMComponent::StateRequestType::IdleRecovery);
				}
			);

			// Animation Component
			scene->CreateComponentWithInit<AnimationComponent>(
				playerObjHandle, [](AnimationComponent* anim) { AnimationLoader::AnimationApply(anim, "CursedKnight", true); }
			);
			
			// Foot IK Component
			scene->CreateComponentWithInit<FootIKComponent>(playerObjHandle, [](FootIKComponent*) {});

			// Shield
			{
				auto shieldHandle = scene->ReserveGameObject("LocalPlayer_Shield");

				scene->CreateComponentWithInit<MeshComponent>(
					shieldHandle,
					[scene, playerObjHandle](MeshComponent* mesh)
					{
						auto res =
							GLOBAL(ResourceGlobal).Load<MeshResource>("Resource/Models/Knight_Armored/Shield.evmesh");
						if (res)
						{
							mesh->SetMeshResource(res);
						}

						if (auto* player = scene->TryGetGameObject(playerObjHandle))
						{
							auto* obj = mesh->GetGameObject();
							obj->GetTransform().SetParent(player->GetTransform().GetHandle());
						}
					}
				);

				scene->CreateComponentWithInit<SocketComponent>(
					shieldHandle,
					[scene, playerObjHandle](SocketComponent* socket)
					{
						if (auto* player = scene->TryGetGameObject(playerObjHandle))
						{
							socket->SetTarget(player, "lowerarm_l");
							// Offset
							float			tx = 0.1634455f;
							float			ty = 0.02278341f;
							float			tz = 0.101984f;
							constexpr float rx = DirectX::XMConvertToRadians(-10.853f);
							constexpr float ry = DirectX::XMConvertToRadians(135.113f);
							constexpr float rz = DirectX::XMConvertToRadians(-251.105f);

							socket->SetOffsetMatrix(
								DirectX::XMMatrixRotationRollPitchYaw(rx, ry, rz) *
								DirectX::XMMatrixTranslation(tx, ty, tz)
							);
						}
					}
				);
			}


			// Sword
			{
				auto swordHandle = scene->ReserveGameObject("LocalPlayer_Sword");

				scene->CreateComponentWithInit<MeshComponent>(
					swordHandle,
					[scene, playerObjHandle](MeshComponent* mesh)
					{
						auto res = GLOBAL(ResourceGlobal).Load<MeshResource>("Resource/Models/Big_Sword.evmesh");
						if (res)
						{
							mesh->SetMeshResource(res);
						}

						if (auto* player = scene->TryGetGameObject(playerObjHandle))
						{
							auto* obj = mesh->GetGameObject();
							obj->GetTransform().SetParent(player->GetTransform().GetHandle());
						}
					}
				);


				scene->CreateComponentWithInit<SocketComponent>(
					swordHandle,
					[scene, playerObjHandle](SocketComponent* socket)
					{
						if (auto* player = scene->TryGetGameObject(playerObjHandle))
						{
							socket->SetTarget(player, "hand_r");

							// Offset
							float			tx = -0.095f;
							float			ty = 0.125f;
							float			tz = -0.043f;
							constexpr float rx = DirectX::XMConvertToRadians(40.563f);
							constexpr float ry = DirectX::XMConvertToRadians(49.596f);
							constexpr float rz = DirectX::XMConvertToRadians(89.208f);

							socket->SetOffsetMatrix(
								DirectX::XMMatrixRotationRollPitchYaw(rx, ry, rz) *
								DirectX::XMMatrixTranslation(tx, ty, tz)
							);
						}
					}
				);

				// Attack Range Debug Indicator
				/*auto attackRangeHandle = scene->ReserveGameObject(
					"LocalPlayer_AttackRangeDebug",
					std::nullopt,
					[scene, playerObjHandle](GameObject* rangeRoot)
					{
						if (auto* player = scene->TryGetGameObject(playerObjHandle))
						{
							rangeRoot->GetTransform().SetParent(player->GetTransform().GetHandle());
							rangeRoot->GetTransform().SetPosition(0.0f, 1.1f, 0.0f);
						}
					}
				);

				for (int segmentIndex = 0; segmentIndex < 18; ++segmentIndex)
				{
					auto segmentHandle = scene->ReserveGameObject(
						"LocalPlayer_AttackRangeDebugSegment",
						std::nullopt,
						[scene, attackRangeHandle](GameObject* segmentObj)
						{
							if (auto* root = scene->TryGetGameObject(attackRangeHandle))
							{
								segmentObj->GetTransform().SetParent(root->GetTransform().GetHandle());
							}
						}
					);

					scene->CreateComponentWithInit<MeshComponent>(
						segmentHandle,
						[](MeshComponent* mesh)
						{
							auto res = GLOBAL(ResourceGlobal).Load<MeshResource>("Resource/Models/Range.evmesh");
							if (!res)
							{
								DEBUG_LOG_FMT("Failed to load attack range mesh resource!\n");
								return;
							}
							mesh->SetMeshResource(res);
						}
					);
				}*/

				//scene->CreateComponentWithInit<AttackRangeDebugComponent>(
				//	attackRangeHandle,
				//	[](AttackRangeDebugComponent* debug)
				//	{
				//		// debug->SetRadius(1.0f);
				//		// debug->SetCenterAngleDegrees(10.0f);
				//	}
				//);
			}
		}
	);

	scene->ReserveGameObject(
		"PlayerCamera", std::nullopt,
		[scene, playerObjHandle](GameObject* camObj)
		{
			auto* playerObj = scene->TryGetGameObject(playerObjHandle);
			auto  playerTrHandle =
				 playerObj ? playerObj->GetComponentHandle<Transform>() : DenseListHandle<Transform>::Invalid();
			auto& tr = camObj->GetTransform();

			scene->CreateComponentWithInit<CameraComponent>(
				camObj->GetHandle(),
				[playerTrHandle](CameraComponent* cam)
				{
					auto* swapChain = GLOBAL(DxRendererGlobal).GetSwapChain();
					float aspect = static_cast<float>(swapChain->GetWidth()) / swapChain->GetHeight();

					cam->SetAsMainCamera();
					cam->SetPerspective(DX::XM_PI / 3.0f, aspect, 0.1f, 1000.0f);
					cam->SetLookAtTarget(playerTrHandle);
					cam->SetFollowTarget(playerTrHandle); // FollowTarget 설정 추가
					cam->SetEnableLookAtRotation(false);
					cam->SetSmoothFollow(true, 10.0f, 10.0f);
					cam->SetFollowOffset(DX::XMFLOAT3{
						CameraConfig::kDefaultLocalOffsetX, CameraConfig::kCameraHeight,
						CameraConfig::kDefaultLocalOffsetZ
					});
					cam->SetFovAnimated(DX::XM_PI / 3.0f, 0.5f);
				}
			);

			scene->CreateComponentWithInit<PlayerControllerComponent>(
				playerObjHandle,
				[camObj](PlayerControllerComponent* controller)
				{
					controller->SetCameraHandle(camObj->GetHandle());
					controller->SetMouseSensitivity(0.1f, 0.1f);
				}
			);
		}
	);

	return true;
}

bool NetBridge::S2C::Handle_SC_ADD_OBJ_PACKET(const SOCKET& socket, const FB_TABLES::SC_ADD_OBJ_PACKET& recvPkt)
{
	// 원격 플레이어 또는 봇 오브젝트 생성
	// TODO: 원격 플레이어 또는 봇 오브젝트를 게임 씬에 생성하고, 필요한 컴포넌트도 추가하기

	// TODO: stanceType 사용하기


	DEBUG_LOG_FMT("[SC_ADD_OBJ_PACKET] \n");
	auto scene = GLOBAL(SceneGlobal).GetActiveScene();

	const uint64 id = recvPkt.obj_id();
	const uint64 localID = scene->GetLocalID();

	if (id == localID)
	{
		DEBUG_LOG_FMT("id is localID: {}\n", id);
		return false;
	}

	auto device = GLOBAL(DxDeviceGlobal).GetDevice();

	const Vec3 pos{recvPkt.pos_info()->pos().x(), recvPkt.pos_info()->pos().y(), recvPkt.pos_info()->pos().z()};
	const Vec3 rot{recvPkt.pos_info()->rot().x(), recvPkt.pos_info()->rot().y(), recvPkt.pos_info()->rot().z()};
	const auto objType = recvPkt.obj_type();
	const auto teamType = recvPkt.team_type();

	std::string objectName;
	switch (objType)
	{
	case FB_ENUMS::GAME_OBJECT_TYPE_PLAYER:
		objectName = "RemotePlayer_" + std::to_string(id); // 다른 플레이어
		break;
	case FB_ENUMS::GAME_OBJECT_TYPE_GENERAL:
		objectName = "Bot_" + std::to_string(id); // NPC 장수
		break;
	default:
		objectName = "GameObject_" + std::to_string(id);
		break;
	}

	// TODO: objType이 점령지일 경우, 내부적으로 dominantTeamType을 가지고 있어야 합니다.(현재 점령중인 팀)
	// SC_OCCUPATION_ZONE_GAUGE_PACKET이 오면 해당 점령지의 게이지 값을 바로 업데이트 해주면 됩니다.
	// 그게 아니라면 매 프레임마다 점령지 오브젝트가 자신의 dominantTeamType을 확인해서 게이지를 5.f만큼 업데이트 해주면
	// 됩니다.

	auto objectHandle = scene->ReserveGameObject(
		objectName, id,
		[scene, pos, rot, objType, teamType, maxHP = recvPkt.max_hp(), currentHP = recvPkt.current_hp(),
		 maxStamina = recvPkt.max_stamina(), currentStamina = recvPkt.current_stamina(), stance = recvPkt.stance_type(),
		 objectName, id](GameObject* obj)
		{
			auto& tr = obj->GetTransform();
			tr.SetPosition(pos.x, pos.y, pos.z);
			tr.SetRotation(rot.x, rot.y, rot.z);

			auto objHandle = obj->GetHandle();

			bool isGeneral = (objType == FB_ENUMS::GAME_OBJECT_TYPE_PLAYER) || objType == FB_ENUMS::GAME_OBJECT_TYPE_GENERAL;

			// MeshComponent 또는 SkinnedMeshComponent 추가
			if (isGeneral)
			{
				scene->CreateComponentWithInit<SkinnedMeshComponent>(
					objHandle,
					[](SkinnedMeshComponent* mesh)
					{
						auto meshRes =
							GLOBAL(ResourceGlobal).Load<SkinnedMeshResource>("Resource/Models/Cursed_Knight.evskin");
						if (nullptr != meshRes)
						{
							mesh->SetSkinnedMeshResource(meshRes);
						}
					}
				);

				// Add Equipment (Belts, Armors, Dress)
				auto addEquipment = [scene, objHandle, objectName](const std::string& name, const std::string& resPath)
				{
					auto handle = scene->ReserveGameObject(objectName + "_" + name);
					scene->CreateComponentWithInit<SkinnedMeshComponent>(
						handle,
						[scene, objHandle, resPath](SkinnedMeshComponent* mesh)
						{
							auto res = GLOBAL(ResourceGlobal).Load<SkinnedMeshResource>(resPath);
							if (res)
							{
								mesh->SetSkinnedMeshResource(res);
							}

							if (auto* parentObj = scene->TryGetGameObject(objHandle))
							{
								auto* obj = mesh->GetGameObject();
								obj->GetTransform().SetParent(parentObj->GetTransform().GetHandle());
								obj->GetTransform().SetPosition(0, 0, 0);
								obj->GetTransform().SetRotation(0, 0, 0);
							}
						}
					);
				};

				addEquipment("Belts", "Resource/Models/Belts.evskin");
				addEquipment("PrimaryArmor", "Resource/Models/Primary_Armors.evskin");
				addEquipment("SecondaryArmor", "Resource/Models/Secondary_Armors.evskin");
				addEquipment("LegsArmor", "Resource/Models/Leg_Armors.evskin");
				// addEquipment("Scarf", "Resource/Models/Scarf.evskin");
				addEquipment("Dress", "Resource/Models/Dress.evskin");

				// BattleUIControllerComponent
				scene->CreateComponentWithInit<BattleUIControllerComponent>(
					objHandle,
					[stance](BattleUIControllerComponent* ui)
					{
						ui->SetControlMode(BattleUIControllerComponent::ControlType::Remote);
						ui->InitStance(stance);
						// DEBUG_LOG_FMT("[BattleUI] Component attached to RemotePlayer. InitStance: {}\n",
						// static_cast<int>(stance));
					}
				);

				// StaminaComponent
				scene->CreateComponentWithInit<StaminaComponent>(
					objHandle,
					[maxStamina, currentStamina](StaminaComponent* stamina)
					{
						stamina->SetMaxStamina(maxStamina);
						stamina->SetStamina(currentStamina);
					}
				);

				// Animation Component
				scene->CreateComponentWithInit<AnimationComponent>(
					objHandle, [](AnimationComponent* anim) { AnimationLoader::AnimationApply(anim, "CursedKnight"); }
				);


				// Shield
				auto shieldHandle = scene->ReserveGameObject(objectName + "_Shield");

				scene->CreateComponentWithInit<MeshComponent>(
					shieldHandle,
					[scene, objHandle](MeshComponent* mesh)
					{
						auto res =
							GLOBAL(ResourceGlobal).Load<MeshResource>("Resource/Models/Knight_Armored/Shield.evmesh");
						if (res)
						{
							mesh->SetMeshResource(res);
						}

						if (auto* parentObj = scene->TryGetGameObject(objHandle))
						{
							auto* obj = mesh->GetGameObject();
							obj->GetTransform().SetParent(parentObj->GetTransform().GetHandle());
						}
					}
				);

				scene->CreateComponentWithInit<SocketComponent>(
					shieldHandle,
					[scene, objHandle](SocketComponent* socket)
					{
						if (auto* targetObj = scene->TryGetGameObject(objHandle))
						{
							socket->SetTarget(targetObj, "lowerarm_l");
							// Offset
							float			tx = 0.1634455f;
							float			ty = 0.02278341f;
							float			tz = 0.101984f;
							constexpr float rx = DirectX::XMConvertToRadians(-10.853f);
							constexpr float ry = DirectX::XMConvertToRadians(135.113f);
							constexpr float rz = DirectX::XMConvertToRadians(-251.105f);

							socket->SetOffsetMatrix(
								DirectX::XMMatrixRotationRollPitchYaw(rx, ry, rz) *
								DirectX::XMMatrixTranslation(tx, ty, tz)
							);
						}
					}
				);

				auto swordHandle = scene->ReserveGameObject(objectName + "_Sword");

				scene->CreateComponentWithInit<MeshComponent>(
					swordHandle,
					[scene, objHandle](MeshComponent* mesh)
					{
						auto res = GLOBAL(ResourceGlobal).Load<MeshResource>("Resource/Models/Sword.evmesh");
						if (res)
						{
							mesh->SetMeshResource(res);
						}

						if (auto* parentObj = scene->TryGetGameObject(objHandle))
						{
							auto* obj = mesh->GetGameObject();
							obj->GetTransform().SetParent(parentObj->GetTransform().GetHandle());
						}
					}
				);

				// Sword
				scene->CreateComponentWithInit<SocketComponent>(
					swordHandle,
					[scene, objHandle](SocketComponent* socket)
					{
						if (auto* targetObj = scene->TryGetGameObject(objHandle))
						{
							socket->SetTarget(targetObj, "hand_r");

							// Offset
							constexpr float tx = -0.095f;
							constexpr float ty = 0.125f;
							constexpr float tz = -0.043f;
							constexpr float rx = DirectX::XMConvertToRadians(40.563f);
							constexpr float ry = DirectX::XMConvertToRadians(49.596f);
							constexpr float rz = DirectX::XMConvertToRadians(89.208f);

							socket->SetOffsetMatrix(
								DirectX::XMMatrixRotationRollPitchYaw(rx, ry, rz) *
								DirectX::XMMatrixTranslation(tx, ty, tz)
							);
						}
					}
				);

				// FSMComponent
				scene->CreateComponentWithInit<FSMComponent>(
					objHandle,
					[objType, id](FSMComponent* fsm)
					{
						fsm->SetObjectType(static_cast<uint8_t>(objType));
						fsm->RequestState(FSMComponent::StateRequestType::IdleRecovery);
						ApplyPendingServerState(id, fsm);
					}
				);

				// Attack Range Debug Indicator
				/*auto attackRangeHandle = scene->ReserveGameObject(
					"LocalPlayer_AttackRangeDebug", std::nullopt,
					[scene, objHandle](GameObject* rangeRoot)
					{
						if (auto* player = scene->TryGetGameObject(objHandle))
						{
							rangeRoot->GetTransform().SetParent(player->GetTransform().GetHandle());
							rangeRoot->GetTransform().SetPosition(0.0f, 1.1f, 0.0f);
						}
					}
				);

				for (int segmentIndex = 0; segmentIndex < 18; ++segmentIndex)
				{
					auto segmentHandle = scene->ReserveGameObject(
						"LocalPlayer_AttackRangeDebugSegment", std::nullopt,
						[scene, attackRangeHandle](GameObject* segmentObj)
						{
							if (auto* root = scene->TryGetGameObject(attackRangeHandle))
							{
								segmentObj->GetTransform().SetParent(root->GetTransform().GetHandle());
							}
						}
					);

					scene->CreateComponentWithInit<MeshComponent>(
						segmentHandle,
						[](MeshComponent* mesh)
						{
							auto res = GLOBAL(ResourceGlobal).Load<MeshResource>("Resource/Models/Range.evmesh");
							if (!res)
							{
								DEBUG_LOG_FMT("Failed to load attack range mesh resource!\n");
								return;
							}
							mesh->SetMeshResource(res);
						}
					);
				}*/

				//scene->CreateComponentWithInit<AttackRangeDebugComponent>(
				//	attackRangeHandle,
				//	[](AttackRangeDebugComponent* debug)
				//	{
				//		debug->SetRadius(1.0f);
				//		debug->SetCenterAngleDegrees(10.0f);
				//	}
				//);
				/////////////

			}
			else if (objType == FB_ENUMS::GAME_OBJECT_TYPE_SOLDIER) ////// Soldier
			{
				tr.SetScale(0.9f);
				scene->CreateComponentWithInit<SkinnedMeshComponent>(
					objHandle,
					[teamType](SkinnedMeshComponent* mesh)
					{
						auto meshRes =
							GLOBAL(ResourceGlobal).Load<SkinnedMeshResource>("Resource/Models/Knight_Armored.evskin");
						if (nullptr != meshRes)
						{
							mesh->SetSkinnedMeshResource(meshRes);
						}
					}
				);

				scene->CreateComponentWithInit<AnimationComponent>(
					objHandle, [](AnimationComponent* anim) { AnimationLoader::AnimationApply(anim, "Knight_Armored"); }
				);

				// FSMComponent
				scene->CreateComponentWithInit<FSMComponent>(
					objHandle,
					[objType, id](FSMComponent* fsm)
					{
						fsm->SetObjectType(static_cast<uint8_t>(objType));
						fsm->SetServerState(FB_ENUMS::SOLDIER_STATE_TYPE_IDLE);
						ApplyPendingServerState(id, fsm);
					}
				);

				// Shield
				auto shieldHandle = scene->ReserveGameObject(objectName + "_Shield");

				scene->CreateComponentWithInit<MeshComponent>(
					shieldHandle,
					[scene, objHandle](MeshComponent* mesh)
					{
						auto res = GLOBAL(ResourceGlobal).Load<MeshResource>("Resource/Models/Shield.evmesh");
						if (res)
						{
							mesh->SetMeshResource(res);
						}

						if (auto* parentObj = scene->TryGetGameObject(objHandle))
						{
							auto* obj = mesh->GetGameObject();
							obj->GetTransform().SetParent(parentObj->GetTransform().GetHandle());
						}
					}
				);

				scene->CreateComponentWithInit<SocketComponent>(
					shieldHandle,
					[scene, objHandle](SocketComponent* socket)
					{
						if (auto* targetObj = scene->TryGetGameObject(objHandle))
						{
							socket->SetTarget(targetObj, "lowerarm_l");
							// Offset
							float			tx = 0.1634455f;
							float			ty = 0.02278341f;
							float			tz = 0.101984f;
							constexpr float rx = DirectX::XMConvertToRadians(-10.853f);
							constexpr float ry = DirectX::XMConvertToRadians(135.113f);
							constexpr float rz = DirectX::XMConvertToRadians(-251.105f);

							socket->SetOffsetMatrix(
								DirectX::XMMatrixRotationRollPitchYaw(rx, ry, rz) *
								DirectX::XMMatrixTranslation(tx, ty, tz)
							);
						}
					}
				);

				// Sword
				auto swordHandle = scene->ReserveGameObject(objectName + "_Sword");

				scene->CreateComponentWithInit<MeshComponent>(
					swordHandle,
					[scene, objHandle](MeshComponent* mesh)
					{
						auto res = GLOBAL(ResourceGlobal).Load<MeshResource>("Resource/Models/Sword.evmesh");
						if (res)
						{
							mesh->SetMeshResource(res);
						}

						if (auto* parentObj = scene->TryGetGameObject(objHandle))
						{
							auto* obj = mesh->GetGameObject();
							obj->GetTransform().SetParent(parentObj->GetTransform().GetHandle());
						}
					}
				);

				scene->CreateComponentWithInit<SocketComponent>(
					swordHandle,
					[scene, objHandle](SocketComponent* socket)
					{
						if (auto* targetObj = scene->TryGetGameObject(objHandle))
						{
							socket->SetTarget(targetObj, "hand_r");

							// Offset
							constexpr float tx = -0.102f;
							constexpr float ty = 0.106f;
							constexpr float tz = -0.033f;
							constexpr float rx = DirectX::XMConvertToRadians(43.896f);
							constexpr float ry = DirectX::XMConvertToRadians(65.171f);
							constexpr float rz = DirectX::XMConvertToRadians(76.545f);

							socket->SetOffsetMatrix(
								DirectX::XMMatrixRotationRollPitchYaw(rx, ry, rz) *
								DirectX::XMMatrixTranslation(tx, ty, tz)
							);
						}
					}
				);

				// Attack Range Debug Indicator
				/*auto attackRangeHandle = scene->ReserveGameObject(
					"LocalPlayer_AttackRangeDebug", std::nullopt,
					[scene, objHandle](GameObject* rangeRoot)
					{
						if (auto* player = scene->TryGetGameObject(objHandle))
						{
							rangeRoot->GetTransform().SetParent(player->GetTransform().GetHandle());
							rangeRoot->GetTransform().SetPosition(0.0f, 1.1f, 0.0f);
						}
					}
				);

				for (int segmentIndex = 0; segmentIndex < 18; ++segmentIndex)
				{
					auto segmentHandle = scene->ReserveGameObject(
						"LocalPlayer_AttackRangeDebugSegment", std::nullopt,
						[scene, attackRangeHandle](GameObject* segmentObj)
						{
							if (auto* root = scene->TryGetGameObject(attackRangeHandle))
							{
								segmentObj->GetTransform().SetParent(root->GetTransform().GetHandle());
							}
						}
					);

					scene->CreateComponentWithInit<MeshComponent>(
						segmentHandle,
						[](MeshComponent* mesh)
						{
							auto res = GLOBAL(ResourceGlobal).Load<MeshResource>("Resource/Models/Range.evmesh");
							if (!res)
							{
								DEBUG_LOG_FMT("Failed to load attack range mesh resource!\n");
								return;
							}
							mesh->SetMeshResource(res);
						}
					);
				}*/

			/*	scene->CreateComponentWithInit<AttackRangeDebugComponent>(
					attackRangeHandle,
					[](AttackRangeDebugComponent* debug)
					{
						debug->SetRadius(1.0f);
						debug->SetCenterAngleDegrees(10.0f);
					}
				);*/
				////
			}

			// MovementComponent 추가 (네트워크 보간을 위해)
			scene->CreateComponentWithInit<MovementComponent>(
				objHandle,
				[](MovementComponent* movement)
				{
					movement->SetMovementMode(MovementMode::Physics);
					movement->SetMoveSpeed(5.0f);
				}
			);

			scene->CreateComponentWithInit<HealthComponent>(
				objHandle,
				[maxHP, currentHP](HealthComponent* health)
				{
					health->SetMaxHealth(maxHP);
					health->SetHealth(currentHP);
				}
			);

			// TeamComponent
			scene->CreateComponentWithInit<TeamComponent>(
				objHandle, [teamType](TeamComponent* team) { team->SetTeamType(teamType); }
			);

			// VitalUIControllerComponent
			scene->CreateComponentWithInit<VitalUIControllerComponent>(
				objHandle,
				[](VitalUIControllerComponent* vital)
				{
					// Init 제거->OnStart에서 자동 판단
				}
			);

			DEBUG_LOG_FMT("Created at ({:.2f}, {:.2f}, {:.2f}), HP: {}/{}\n", pos.x, pos.y, pos.z, currentHP, maxHP);
		}
	);

	return true;
}

bool NetBridge::S2C::Handle_SC_REMOVE_OBJ_PACKET(const SOCKET& socket, const FB_TABLES::SC_REMOVE_OBJ_PACKET& recvPkt)
{
	// 원격 플레이어 또는 봇 오브젝트 제거
	// TODO: 원격 플레이어 또는 봇 오브젝트를 게임 씬에서 제거하기
	auto scene = GLOBAL(SceneGlobal).GetActiveScene();

	const uint64 id = recvPkt.obj_id();
	const uint64 localID = scene->GetLocalID();

	if (id == localID)
	{
		return false;
	}

	if (auto obj = scene->FindGameObjectByServerID(id))
	{
		scene->DestroyGameObject(obj->GetHandle());
		DEBUG_LOG_FMT("ID: {}, Remove!\n", id);
		obj->SetActive(false);
	}


	return true;
}

bool NetBridge::S2C::Handle_SC_CHAT_PACKET(const SOCKET& socket, const FB_TABLES::SC_CHAT_PACKET& recvPkt)
{
	// 채팅 메시지 수신
	// TODO: 채팅 메시지를 게임 내 채팅 창에 표시하기
	std::cout << recvPkt.msg()->c_str() << std::endl;
	return true;
}

bool NetBridge::S2C::Handle_SC_MOVE_PACKET(const SOCKET& socket, const FB_TABLES::SC_MOVE_PACKET& recvPkt)
{
	auto scene = GLOBAL(SceneGlobal).GetActiveScene();

	const uint64 id = recvPkt.obj_id();
	const uint64 localID = scene->GetLocalID();

	if (id == localID)
	{
		return true;
	}

	auto obj = scene->FindGameObjectByServerID(id);
	if (!obj)
		return false;

	const Vec3 pos{recvPkt.pos_info()->pos().x(), recvPkt.pos_info()->pos().y(), recvPkt.pos_info()->pos().z()};
	const Vec3 rot{recvPkt.pos_info()->rot().x(), recvPkt.pos_info()->rot().y(), recvPkt.pos_info()->rot().z()};
	auto*	   fsm = obj->GetComponent<FSMComponent>();

	if (fsm &&
		fsm->GetObjectType() == static_cast<uint8_t>(FB_ENUMS::GAME_OBJECT_TYPE_SOLDIER) &&
		fsm->GetCurStateType() == StateOffset::kSoldierOffset + static_cast<uint8_t>(FB_ENUMS::SOLDIER_STATE_TYPE_DEAD))
	{
		return true;
	}

	// obj->GetTransform().SetPosition(pos);
	// obj->GetTransform().SetRotation(rot);

	// MovementComponent가 있으면 네트워크 보간으로 부드럽게 이동, 없으면 즉시 스냅
	if (auto* movement = obj->GetComponent<MovementComponent>())
	{
		movement->SetNetInterpTarget(pos, rot);
	}
	else
	{
		obj->GetTransform().SetPosition(pos);
		obj->GetTransform().SetRotation(rot);
	}

	if (id != localID)
	{
		// 서버에서 보내준 state를 FSM에 전달
		if (fsm)
		{
			fsm->SetMoveDirection(recvPkt.move_dir());
		}
	}
	return true;
}

bool NetBridge::S2C::Handle_SC_GENERAL_ATTACK_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_GENERAL_ATTACK_PACKET& recvPkt
)
{
	auto		 scene = GLOBAL(SceneGlobal).GetActiveScene();
	const uint64 id = recvPkt.obj_id();
	const uint64 localID = scene->GetLocalID();

	// 서버 Echo 방지 (로컬 플레이어는 이미 입력 시점에 처리)
	if (id == localID)
	{
		return true;
	}

	const auto attackInfo = recvPkt.attack_info();
	if (!attackInfo)
	{
		return false;
	}

	const auto type = attackInfo->attack_type();
	const auto dir = attackInfo->attack_dir();

	// 1. 공격자 오브젝트 찾기
	if (auto* obj = scene->FindGameObjectByServerID(id))
	{
		// 2. 컴포넌트 가져오기
		if (auto* uiController = obj->GetComponent<BattleUIControllerComponent>())
		{
			// UI 갱신
			uiController->TriggerAttackRemote(type, dir);
			// DEBUG_LOG_FMT("[SC_PLAYER_ATTACK] ID: {}, Type: {}, Dir: {}\n", id, static_cast<int>(type),
			// static_cast<int>(dir));

			// FSM 상태 동기화: 공격 타입 설정
			if (auto* fsm = obj->GetComponent<FSMComponent>())
			{
				fsm->SetCurAttackType(static_cast<GENERAL_ATTACK_TYPE>(type));
				const auto objectType = fsm->GetObjectType();
				if (
					objectType == static_cast<uint8_t>(FB_ENUMS::GAME_OBJECT_TYPE_GENERAL))
				{
					fsm->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_ATTACK);
				}
			}
			return true;
		}
	}

	return false;
}

bool NetBridge::S2C::Handle_SC_UPDATE_VITAL_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_UPDATE_VITAL_PACKET& recvPkt
)
{
	// 오브젝트의 체력 정보 업데이트f
	// TODO: 해당 오브젝트의 HealthComponent를 찾아서 바이탈 정보 업데이트하기
	auto scene = GLOBAL(SceneGlobal).GetActiveScene();

	const uint64 objID = recvPkt.obj_id();
	auto		 obj = scene->FindGameObjectByServerID(objID);

	if (obj)
	{
		const uint32 hp = recvPkt.current_hp();
		auto		 healthComp = obj->GetComponent<HealthComponent>();
		if (healthComp)
		{
			healthComp->SetHealth(hp);
		}

		const uint32 stamina = recvPkt.current_stamina();


		// std::cout << std::format("ID:{}, hp:{}, stanmina:{}", objID, hp, stamina);

		auto staminaComp = obj->GetComponent<StaminaComponent>();
		if (staminaComp)
		{
			staminaComp->SetStamina(stamina);
		}
	}

	return true;
}

bool NetBridge::S2C::Handle_SC_CHANGE_GENERAL_STANCE_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_CHANGE_GENERAL_STANCE_PACKET& recvPkt
)
{
	auto		 scene = GLOBAL(SceneGlobal).GetActiveScene();
	const uint64 id = recvPkt.obj_id();
	const auto	 stance = recvPkt.stance_type();

	// 서버 Echo 방지
	if (id == scene->GetLocalID())
	{
		auto cameraComp = CameraComponent::GetMainCamera();

		if (!cameraComp)
		{
			return false;
		}

		const auto cameraTargetID = recvPkt.camera_target_id();

		if (cameraTargetID == 0)
		{
			// 락온 해제 시(적 죽었을 때) 로컬 플레이어로 복구
			const uint64 localID = scene->GetLocalID();
			if (auto localPlayer = scene->FindGameObjectByServerID(localID))
			{
				cameraComp->SetLookAtTarget(localPlayer->GetHandle());
				cameraComp->SetLookAtTargetOffset({0.0f, 0.0f, 0.0f}); // 오프셋 초기화
				cameraComp->SetEnableLookAtRotation(false); // 자유 시점
				cameraComp->SetFollowOffsetLocal(
					{CameraConfig::kDefaultLocalOffsetX, CameraConfig::kCameraHeight, CameraConfig::kDefaultLocalOffsetZ
					}
				); // 오프셋 복구 (공유 상수 사용)
				DEBUG_LOG_FMT("[SC_CHANGE_CAMERA_TARGET_PACKET] Camera Reset to LocalPlayer\n");
			}
			else
			{
				cameraComp->ClearLookAtTarget();
				cameraComp->SetLookAtTargetOffset({0.0f, 0.0f, 0.0f});
				DEBUG_LOG_FMT("[SC_CHANGE_CAMERA_TARGET_PACKET] Camera Target Cleared (LocalPlayer not found)\n");
			}
		}
		else
		{
			if (auto targetObj = scene->FindGameObjectByServerID(cameraTargetID))
			{
				cameraComp->SetLookAtTarget(targetObj->GetHandle());
				cameraComp->SetLookAtTargetOffset({0.0f, CameraConfig::kLockOnViewOffsetY, 0.0f}); // 대상을 바라볼 때 약간 위를 바라보도록 설정
				cameraComp->SetEnableLookAtRotation(true); // 락온 시에 회전 고정
				DEBUG_LOG_FMT("[SC_CHANGE_CAMERA_TARGET_PACKET] Camera Target Set to ID: {}\n", cameraTargetID);
			}
			else
			{
				DEBUG_LOG_FMT("[SC_CHANGE_CAMERA_TARGET_PACKET] Target ID {} not found in scene\n", cameraTargetID);
			}
		}

		return true;
	}

	if (auto* obj = scene->FindGameObjectByServerID(id))
	{
		if (auto* fsm = obj->GetComponent<FSMComponent>())
		{
			if (fsm->IsLockOn())
				fsm->SetLockOn(false);
			else
				fsm->SetLockOn(true);
			fsm->SetStance(static_cast<uint8_t>(stance));
			DEBUG_LOG_FMT("[SC_CHANGE_PLAYER_STANCE] ID: {}, Stance: {}\n", id, static_cast<int>(stance));
		}
	}

	return true;
}

bool NetBridge::S2C::Handle_SC_UPDATE_STATE_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_UPDATE_STATE_PACKET& recvPkt
)
{
	auto scene = GLOBAL(SceneGlobal).GetActiveScene();
	if (!scene)
	{
		return false;
	}

	auto		 localID{GLOBAL(SceneGlobal).GetLocalGameObjectID()};
	const uint64 objID = recvPkt.obj_id();
	auto		 obj = scene->FindGameObjectByServerID(objID);
	uint8_t		 nextState = recvPkt.next_state();

	if (localID == objID)
	{
		goto SET_LOCAL;
	}

	if (!obj)
	{
		s_pendingStateByObjectID[objID] = nextState;
		return true;
	}

	// FSM 상태 동기화
	if (auto* fsm = obj->GetComponent<FSMComponent>())
	{
		//DEBUG_LOG_FMT(
		//	"[RemoteState] objID={}, received={}, before={}\n",
		//	objID,
		//	static_cast<int>(nextState),
		//	static_cast<int>(fsm->GetCurStateType())
		//);

		if (fsm->GetObjectType() == static_cast<uint8_t>(FB_ENUMS::GAME_OBJECT_TYPE_SOLDIER) && nextState == FB_ENUMS::SOLDIER_STATE_TYPE_ATTACK)
		{
			return true;
		}
		if (fsm->GetObjectType() == static_cast<uint8_t>(FB_ENUMS::GAME_OBJECT_TYPE_GENERAL) && nextState == FB_ENUMS::GENERAL_STATE_TYPE_ATTACK)
		{
			return true;
		}
		fsm->SetServerState(nextState);
		//DEBUG_LOG_FMT(
		//	"[RemoteState] objID={}, after={}\n",
		//	objID,
		//	static_cast<int>(fsm->GetCurStateType())
		//);
		return true;
	}

	s_pendingStateByObjectID[objID] = nextState;
	return true;

SET_LOCAL:
	if (auto* fsm = obj->GetComponent<FSMComponent>())
	{
		if (nextState == FB_ENUMS::PLAYER_STATE_TYPE_STUN || nextState == FB_ENUMS::PLAYER_STATE_TYPE_DEAD)
		{
			fsm->SetServerState(nextState);
			return true;
		}
	}

	return true;
}

bool NetBridge::S2C::Handle_SC_REMAINING_GAME_TIME_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_REMAINING_GAME_TIME& recvPkt
)
{
	// 게임 남은 시간 정보 수신
	// TODO: 게임 남은 시간을 화면에 표시하기

	const uint32   remainingTime{recvPkt.remaining_time()};
	const uint32_t totalSeconds = remainingTime;
	const uint32_t minutes = totalSeconds / 60;
	const uint32_t seconds = totalSeconds % 60;

	if (auto* text = FindRemainingTimeText(GLOBAL(SceneGlobal).GetActiveScene()))
	{
		wchar_t buffer[16]{};
		swprintf_s(buffer, L"%02u:%02u", minutes, seconds);
		text->SetText(buffer);
	}

	return true;
}

bool NetBridge::S2C::Handle_SC_CHANGE_CAMERA_TARGET_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_CHANGE_CAMERA_TARGET_PACKET& recvPkt
)
{
	// 카메라 타겟 변경
	auto scene = GLOBAL(SceneGlobal).GetActiveScene();
	auto cameraComp = CameraComponent::GetMainCamera();

	if (!cameraComp)
	{
		return false;
	}

	const auto cameraTargetID = recvPkt.camera_target_id();
	BattleUIControllerComponent::SetLockedTargetID(0);

	if (cameraTargetID == 0)
	{
		// 락온 해제 시(적 죽었을 때) 로컬 플레이어로 복구
		const uint64 localID = scene->GetLocalID();
		if (auto localPlayer = scene->FindGameObjectByServerID(localID))
		{
			cameraComp->SetLookAtTarget(localPlayer->GetHandle());
			cameraComp->SetLookAtTargetOffset({0.0f, 0.0f, 0.0f}); // 오프셋 초기화
			cameraComp->SetEnableLookAtRotation(false); // 자유 시점
			cameraComp->SetFollowOffsetLocal(
				{CameraConfig::kDefaultLocalOffsetX, CameraConfig::kCameraHeight, CameraConfig::kDefaultLocalOffsetZ}
			); // 오프셋 복구 (공유 상수 사용)
			DEBUG_LOG_FMT("[SC_CHANGE_CAMERA_TARGET_PACKET] Camera Reset to LocalPlayer\n");
		}
		else
		{
			cameraComp->ClearLookAtTarget();
			cameraComp->SetLookAtTargetOffset({0.0f, 0.0f, 0.0f});
			DEBUG_LOG_FMT("[SC_CHANGE_CAMERA_TARGET_PACKET] Camera Target Cleared (LocalPlayer not found)\n");
		}
	}
	else
	{
		if (auto targetObj = scene->FindGameObjectByServerID(cameraTargetID))
		{
			cameraComp->SetLookAtTarget(targetObj->GetHandle());
			cameraComp->SetLookAtTargetOffset({0.0f, CameraConfig::kLockOnViewOffsetY, 0.0f}
			);										   // 대상을 바라볼 때 약간 위를 바라보도록 설정
			cameraComp->SetEnableLookAtRotation(true); // 락온 시에 회전 고정
			BattleUIControllerComponent::SetLockedTargetID(cameraTargetID);
			DEBUG_LOG_FMT("[SC_CHANGE_CAMERA_TARGET_PACKET] Camera Target Set to ID: {}\n", cameraTargetID);
		}
		else
		{
			DEBUG_LOG_FMT("[SC_CHANGE_CAMERA_TARGET_PACKET] Target ID {} not found in scene\n", cameraTargetID);
		}
	}

	return true;
}

bool NetBridge::S2C::Handle_SC_CHANGE_GENERAL_ATTACK_DIR_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_CHANGE_GENERAL_ATTACK_DIR_PACKET& recvPkt
)
{
	// 플레이어 공격 방향 표시
	auto scene = GLOBAL(SceneGlobal).GetActiveScene();

	const auto id = recvPkt.obj_id();
	const auto localID = scene->GetLocalID();

	// 서버 Echo 방지
	if (id == localID)
	{
		return true;
	}

	const uint8_t				  dirRaw = recvPkt.attack_dir();
	const GENERAL_ATTACK_DIR_TYPE dir = static_cast<GENERAL_ATTACK_DIR_TYPE>(dirRaw);

	// 1. 플레이어 오브젝트 찾기
	if (auto* playerObj = scene->FindGameObjectByServerID(id))
	{
		// 2. 컴포넌트 가져오기
		if (auto* uiController = playerObj->GetComponent<BattleUIControllerComponent>())
		{
			// 3. UI 갱신 (Observer - UI 자동 업데이트)
			uiController->UpdateUISelection(dir, std::nullopt);
			return true;
		}
	}

	return false;
}

bool NetBridge::S2C::Handle_SC_RESPAWN_GENERAL_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_RESPAWN_GENERAL_PACKET& recvPkt
)
{
	auto	   scene = GLOBAL(SceneGlobal).GetActiveScene();
	const auto objID = recvPkt.obj_id();
	auto	   obj = scene->FindGameObjectByServerID(objID);

	if (obj)
	{
		// 부활
		obj->SetActive(true);

		// 위치, 회전 업데이트
		const Vec3 pos{recvPkt.pos_info()->pos().x(), recvPkt.pos_info()->pos().y(), recvPkt.pos_info()->pos().z()};
		const Vec3 rot{recvPkt.pos_info()->rot().x(), recvPkt.pos_info()->rot().y(), recvPkt.pos_info()->rot().z()};
		obj->GetTransform().SetPosition(pos);
		obj->GetTransform().SetRotation(rot);

		// 바이탈 업데이트
		if (auto* health = obj->GetComponent<HealthComponent>())
		{
			health->SetMaxHealth(recvPkt.max_hp());
			health->SetHealth(recvPkt.current_hp());
		}
		if (auto* stamina = obj->GetComponent<StaminaComponent>())
		{
			stamina->SetMaxStamina(recvPkt.max_stamina());
			stamina->SetStamina(recvPkt.current_stamina());
		}

		// 스탠스 업데이트
		if (auto* fsm = obj->GetComponent<FSMComponent>())
		{
			fsm->SetStance(static_cast<uint8_t>(recvPkt.stance_type()));
			fsm->RequestState(FSMComponent::StateRequestType::IdleRecovery);
		}
		// 디버깅
		DEBUG_LOG_FMT(
			"[SC_RESPAWN_GENERAL_PACKET] Object ID {} has respawned at ({:.1f}, {:.1f}, {:.1f})\n", objID, pos.x, pos.y,
			pos.z
		);

		return true;
	}

	DEBUG_LOG_FMT("[SC_RESPAWN_GENERAL_PACKET] Failed to find object ID {}\n", objID);
	return false;
}

bool NetBridge::S2C::Handle_SC_PING_PACKET(const SOCKET& socket, const FB_TABLES::SC_PING_PACKET& recvPkt)
{
	// Ping 패킷 수신 시, Pong 패킷 전송
	auto pb = C2S::Make_CS_PONG_PACKET();
	GLOBAL(NetworkGlobal).Send(std::move(pb));
	return true;
}

bool NetBridge::S2C::Handle_SC_GAME_FINISH_PACKET(const SOCKET& socket, const FB_TABLES::SC_GAME_FINISH_PACKET& recvPkt)
{
	const uint32 sessionID = GLOBAL(SceneGlobal).GetSessionID();
	ScoreScene::SetScores(s_latestRedScore, s_latestBlueScore);
	GLOBAL(SceneGlobal).LoadScene("ScoreScene");

	GLOBAL(NetBridge::NetworkGlobal).DisconnectGameServer();
	if (false == GLOBAL(NetBridge::NetworkGlobal).ReconnectLobbyServer())
	{
		DEBUG_LOG_FMT("Failed to reconnect lobby server.\n");
		return false;
	}

	auto pb{NetBridge::C2S::Make_CL_RETURN_TO_GAME_ROOM_PACKET(sessionID)};
	GLOBAL(NetBridge::NetworkGlobal).SendLobby(std::move(pb));
	return true;
}

bool NetBridge::S2C::Handle_SC_UPDATE_TEAM_SCORE_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_UPDATE_TEAM_SCORE_PACKET& recvPkt
)
{
	s_latestBlueScore = recvPkt.blue_score();
	s_latestRedScore = recvPkt.red_score();
	std::cout << std::format("Blue Team: {}, Red Team: {}\n", recvPkt.blue_score(), recvPkt.red_score()) << std::endl;
	return true;
}

bool NetBridge::S2C::Handle_SC_OCCUPATION_ZONE_OCCUPIED_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_OCCUPATION_ZONE_OCCUPIED_PACKET& recvPkt
)
{
	// TODO: ID로 점령지 오브젝트 찾아서 점령 상태 업데이트하기

	return true;
}

bool NetBridge::S2C::Handle_SC_SOLDIER_ATTACK_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_SOLDIER_ATTACK_PACKET& recvPkt
)
{
	auto scene = GLOBAL(SceneGlobal).GetActiveScene();
	if (!scene)
	{
		return false;
	}

	auto		 localID{GLOBAL(SceneGlobal).GetLocalGameObjectID()};
	const uint64 objID = recvPkt.obj_id();
	auto		 obj = scene->FindGameObjectByServerID(objID);

	if (localID == objID)
	{
		return false;
	}

	if (!obj)
	{
		return false;
	}

	// FSM 상태 동기화
	if (auto* fsm = obj->GetComponent<FSMComponent>())
	{
		// DEBUG_LOG_FMT("[S2C] State Update - ID: {}, NextState: {}\n", objID, static_cast<int>(nextState));
		fsm->SetServerState(FB_ENUMS::SOLDIER_STATE_TYPE_ATTACK);
		return true;
	}

	return true;
}

bool NetBridge::S2C::Handle_SC_GENERAL_GUARD_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_GENERAL_GUARD_PACKET& recvPkt
)
{
	auto scene = GLOBAL(SceneGlobal).GetActiveScene();
	if (!scene)
	{
		return false;
	}

	const uint64 defenderID = recvPkt.defender_id();
	const uint64 attackerID = recvPkt.attacker_id();

	if (auto* defenderObj = scene->FindGameObjectByServerID(defenderID))
	{
		if (auto* fsm = defenderObj->GetComponent<FSMComponent>())
		{
			fsm->SetGuardRole(FSMComponent::GuardRole::Defender);
			fsm->RequestState(
				FSMComponent::StateRequestType::Guard,
				static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_GUARD)
			);
		}
	}

	if (auto* attackerObj = scene->FindGameObjectByServerID(attackerID))
	{
		if (auto* fsm = attackerObj->GetComponent<FSMComponent>())
		{
			fsm->SetGuardRole(FSMComponent::GuardRole::Attacker);
			fsm->RequestState(
				FSMComponent::StateRequestType::Guard,
				static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_GUARD)
			);
		}
	}

	return true;
}

bool NetBridge::S2C::Handle_SC_HIT_SOUND_PACKET(const SOCKET& socket, const FB_TABLES::SC_HIT_SOUND_PACKET& recvPkt)
{
	const auto attackerID{recvPkt.attacker_id()};
	// TODO: 내 로컬 아이디와 attackerID 일치하면 HitSound 재생

	return true;
}

bool NetBridge::S2C::Handle_SC_TELEPORT_PACKET(const SOCKET& socket, const FB_TABLES::SC_TELEPORT_PACKET& recvPkt)
{
	auto scene = GLOBAL(SceneGlobal).GetActiveScene();

	const uint64 id = recvPkt.obj_id();
	const uint64 localID = scene->GetLocalID();

	// TODO: obj의 이전 위치와 현재 받은 위치를 이용해서 보간 처리해야 함

	auto obj = scene->FindGameObjectByServerID(id);
	if (!obj)
		return false;

	const Vec3 pos{recvPkt.pos_info()->pos().x(), recvPkt.pos_info()->pos().y(), recvPkt.pos_info()->pos().z()};
	const Vec3 rot{recvPkt.pos_info()->rot().x(), recvPkt.pos_info()->rot().y(), recvPkt.pos_info()->rot().z()};
	obj->GetTransform().SetPosition(pos);
	obj->GetTransform().SetRotation(rot);

	if (id != localID)
	{
		// 서버에서 보내준 state를 FSM에 전달
		if (auto* fsm = obj->GetComponent<FSMComponent>())
		{
			fsm->SetMoveDirection(recvPkt.move_dir());
		}
	}
	return true;
}

bool NetBridge::S2C::Handle_SC_OCCUPATION_ZONE_GAUGE_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_OCCUPATION_ZONE_GAUGE_PACKET& recvPkt
)
{
	auto* scene = GLOBAL(SceneGlobal).GetActiveScene();
	if (!scene)
	{
		return false;
	}

	const float gauge = std::clamp(recvPkt.occupy_gauge(), -100.0f, 100.0f);
	const float blueAmount = std::max(-gauge, 0.0f) / 100.0f;
	const float redAmount = std::max(gauge, 0.0f) / 100.0f;

	for (auto& rect : scene->GetStorage<RectTransformComponent>()->GetList())
	{
		auto* owner = rect.GetGameObject();
		if (!owner)
		{
			continue;
		}

		if (owner->GetName() == "OccupationGaugeBlue")
		{
			rect.SetAnchors({0.5f - 0.5f * blueAmount, 0.0f}, {0.5f, 1.0f});
			rect.SetOffsetMin({0.0f, 2.0f});
			rect.SetOffsetMax({0.0f, -2.0f});
		}
		else if (owner->GetName() == "OccupationGaugeRed")
		{
			rect.SetAnchors({0.5f, 0.0f}, {0.5f + 0.5f * redAmount, 1.0f});
			rect.SetOffsetMin({0.0f, 2.0f});
			rect.SetOffsetMax({0.0f, -2.0f});
		}
	}

	return true;
}

