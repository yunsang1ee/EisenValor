#include "stdafxClient.h"
#include "S2CPackets.h"
#include "C2SPackets.h"
#include "DxDeviceGlobal.h"
#include "SceneGlobal.h"
#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"

// Resource
#include "ResourceGlobal.h"
#include "MeshComponent.h"
#include "MeshResource.h"

#include "CameraComponent.h"
#include "MovementComponent.h"
#include "DxSwapChain.h"
#include "DxRendererGlobal.h"
#include "Component/PlayerControllerComponent.h"
#include "Component/HealthComponent.h"
#include "Component/BattleUIControllerComponent.h"
#include "Component/TeamComponent.h"
#include "Component/VitalUIControllerComponent.h"
#include "Component/StaminaComponent.h"
#include "Component/FSM/FSMComponent.h"
#include "Component/FSM/GeneralStates.h"
#include "RectTransformComponent.h"
#include "ImageUIComponent.h"
#include "ButtonUIComponent.h"
#include "ResourceGlobal.h"
#include "MeshResource.h"


using namespace NetBridge;

bool NetBridge::S2C::Handle_Invalid(const SOCKET&, const char* const, const PacketHeader&)
{
	return false;
}

bool NetBridge::S2C::Handle_SC_LOGIN_SUCCESS_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_LOGIN_SUCCESS_PACKET& recvPkt
)
{
	const uint32 id = recvPkt.player_id();

	auto device = GLOBAL(DxDeviceGlobal).GetDevice();

	GLOBAL(SceneGlobal).SetLocalNetworkID(id);

	// TODO: 들어갈 수 Room 목록 중, ROOM 선택해서 들어갈 수 있게끔..
	const uint16 roomID{1};

	// auto pb = NetBridge::C2S::Make_CS_JOIN_GAME_ROOM_PACKET(1000);
	// MANAGER(NetBridge::NetworkManager)->Send(std::move(pb));

	// auto pb = NetBridge::C2S::Make_CS_ENTER_GAME_LOBBY_PACKET();
	// MANAGER(NetBridge::NetworkManager)->Send(std::move(pb));

	// TODO: 로비 씬으로 전환

	// 테스트용으로 바로 게임 월드 진입
	auto pb = C2S::Make_CS_ENTER_GAME_WORLD_PACKET(roomID);
	GLOBAL(NetworkGlobal).Send(std::move(pb));

	DEBUG_LOG_FMT("[SC_LOGIN_SUCCESS_PACKET] id: {}\n", id);
	return true;
}

#pragma region LOBBY & ROOM
bool NetBridge::S2C::Handle_SC_ENTER_GAME_LOBBY_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_ENTER_GAME_LOBBY_PACKET& recvPkt
)
{
	std::cout << "[SC_ENTER_GAME_LOBBY_PACKET] " << std::endl;

	const uint32 localID = GLOBAL(SceneGlobal).GetLocalNetworkID();

	const auto& rooms = recvPkt.rooms();
	const auto& users = recvPkt.users();
	const auto& vecUserID = recvPkt.vec_user_id();

	if (rooms == nullptr)
	{
		return true;
	}

	// 로비에 성공적으로 입장
	// TODO: 로비 씬으로 전환
	// TODO: 로비 씬 안에서 방 목록 및 유저 목록을 화면에 보여주기

	for (const auto* room : *rooms)
	{
		auto roomId = room->id();
		DEBUG_LOG_FMT("Room ID: {}\n", roomId);
	}

	for (flatbuffers::uoffset_t i = 0; i < users->size(); ++i)
	{
		const auto* user = users->Get(i);
		if (vecUserID->Get(i) == localID)
		{
			continue;
		}
		DEBUG_LOG_FMT("-Name:{}, ID:{}\n", user->c_str(), vecUserID->Get(i));
	}

	// auto pb = NetBridge::C2S::Make_CS_JOIN_GAME_ROOM_PACKET(1000);
	// GLOBAL(NetBridge::NetworkManager)->Send(std::move(pb));/

	return true;
}

bool NetBridge::S2C::Handle_SC_ENTER_USER_IN_GAME_LOBBY_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_ADD_USER_IN_GAME_LOBBY_PACKET& recvPkt
)
{
	// 로비에 새로운 유저가 입장
	// TODO: 로비에 새로운 유저가 입장했음을 화면에 보여주기

	DEBUG_LOG_FMT("[SC_ENTER_USER_IN_GAME_LOBBY_PACKET] ");
	DEBUG_LOG_FMT("User Name: {}\n", recvPkt.user_name()->c_str());

	return true;
}

bool NetBridge::S2C::Handle_SC_LEAVE_GAME_LOBBY_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_LEAVE_GAME_LOBBY_PACKET& recvPkt
)
{
	DEBUG_LOG_FMT("[SC_LEAVE_GAME_LOBBY_PACKET] ");
	DEBUG_LOG_FMT("Leave Lobby!\n");
	// TODO: 로그인 Scene으로 다시...
	return false;
}

bool NetBridge::S2C::Handle_SC_REMOVE_USER_IN_GAME_LOBBY_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_REMOVE_USER_IN_GAME_LOBBY_PACKET& recvPkt
)
{
	// 로비에서 유저가 퇴장
	// TODO: 로비에서 유저가 퇴장했음을 화면에 보여주기
	DEBUG_LOG_FMT("[SC_REMOVE_USER_IN_GAME_LOBBY_PACKET] ");
	DEBUG_LOG_FMT("Remove User In Lobby! id = {}\n", recvPkt.user_id());
	return false;
}

bool NetBridge::S2C::Handle_SC_MAKE_GAME_ROOM_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_MAKE_GAME_ROOM_PACKET& recvPkt
)
{
	// 게임 룸이 성공적으로 생성됨
	// TODO: 방이 생성되었음을 화면에 보여주기
	DEBUG_LOG_FMT("[SC_MAKE_GAME_ROOM_PACKET] ");
	const auto& roomInfo = recvPkt.room_info();
	DEBUG_LOG_FMT("Room ID: {}\n", roomInfo->id());
	return true;
}

bool NetBridge::S2C::Handle_SC_JOIN_GAME_ROOM_FAIL_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_JOIN_GAME_ROOM_FAIL_PACKET& recvPkt
)
{
	// 게임 룸 입장 실패
	// TODO: 게임 룸 입장 실패 메시지 화면에 보여주기
	DEBUG_LOG_FMT("[SC_JOIN_GAME_ROOM_FAIL_PACKET] ");
	DEBUG_LOG_FMT("Fail Reason: {}\n", recvPkt.fail_msg()->c_str());
	return true;
}

bool NetBridge::S2C::Handle_SC_JOIN_GAME_ROOM_SUCCESS_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_JOIN_GAME_ROOM_SUCCESS_PACKET& recvPkt
)
{
	// 게임 룸 입장 성공
	// TODO: 게임 룸 입장 성공 메시지 화면에 보여주기
	// TODO: 룸 내부 Scene으로 전환
	// TODO: 룸 내부의 참가자 목록도 및 참여자 정보도 같이 보여주기

	DEBUG_LOG_FMT("[SC_JOIN_GAME_ROOM_SUCCESS_PACKET] ");
	auto user = recvPkt.user();
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

	if (user->team_type() == FB_ENUMS::TEAM_TYPE_OFFENSE)
	{
		DEBUG_LOG_FMT("User TeamType: OFFENSE\n");
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
		if (participant->team_type() == FB_ENUMS::TEAM_TYPE_OFFENSE)
		{
			DEBUG_LOG_FMT("Participant TeamType: OFFENSE\n");
		}
		else
			DEBUG_LOG_FMT("Participant TeamType: DEFENSE\n");
	}

	// auto pb{NetBridge::C2S::Make_CS_START_GAME_PACKET()};
	// GLOBAL(NetBridge::NetworkManager)->Send(std::move(pb));

	return true;
}

bool NetBridge::S2C::Handle_SC_LEAVE_GAME_ROOM_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_LEAVE_GAME_ROOM_PACKET& recvPkt
)
{
	// 게임 룸 퇴장 성공
	// TODO: 로비 Scene으로 전환

	DEBUG_LOG_FMT("[SC_LEAVE_GAME_ROOM_PACKET] ");
	DEBUG_LOG_FMT("Leave Game Room!\n");

	// auto pb = NetBridge::C2S::Make_CS_ENTER_GAME_LOBBY_PACKET();
	// GLOBAL(NetBridge::NetworkManager)->Send(std::move(pb));

	return true;
}

bool NetBridge::S2C::Handle_SC_JOIN_PARTICIPANT_IN_GAME_ROOM_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_JOIN_PARTICIPANT_IN_GAME_ROOM_PKT& recvPkt
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

	if (participant->team_type() == FB_ENUMS::TEAM_TYPE_OFFENSE)
	{
		DEBUG_LOG_FMT("Participant TeamType: OFFENSE\n");
	}
	else
		DEBUG_LOG_FMT("Participant TeamType: DEFENSE\n");


	return true;
}

bool NetBridge::S2C::Handle_SC_READY_GAME_PACKET(const SOCKET& socket, const FB_TABLES::SC_READY_GAME_PACKET& recvPkt)
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

bool NetBridge::S2C::Handle_SC_CHANGE_TEAM_PACKET(const SOCKET& socket, const FB_TABLES::SC_CHANGE_TEAM_PACKET& recvPkt)
{
	// 참가자가 팀을 변경함
	// TODO: 참가자가 팀을 변경했음을 화면에 보여주기
	if (recvPkt.team_type() == FB_ENUMS::TEAM_TYPE_OFFENSE)
	{
		DEBUG_LOG_FMT("User ID: {} Change Team To OFFENSE\n", recvPkt.user_id());
	}
	else
	{
		DEBUG_LOG_FMT("User ID: {} Change Team To DEFENSE\n", recvPkt.user_id());
	}
	return true;
}

bool NetBridge::S2C::Handle_SC_LOADING_GAME_WORLD_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_LOADING_GAME_WORLD_PACKET& recvPkt
)
{
	// TODO: 로딩 중 메시지 화면에 보여주기
	DEBUG_LOG_FMT("[SC_LOADING_GAME_WORLD_PACKET] ");
	Sleep(3000);
	// TODO: 여기서 게임 씬으로 전환
	auto pb = C2S::Make_CS_COMPLETE_LOADING_GAME_WORLD_PACKET();
	GLOBAL(NetworkGlobal).Send(std::move(pb));

	return true;
}

bool NetBridge::S2C::Handle_SC_CHANGE_GAME_ROOM_STATE_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_CHANGE_GAME_ROOM_STATE_PACKET& recvPkt
)
{
	// 게임 룸의 상태가 변경됨
	// TODO: 게임 룸의 상태가 변경되었음을 화면에 보여주기
	DEBUG_LOG_FMT("[SC_CHANGE_GAME_ROOM_STATE_PACKET] ");
	DEBUG_LOG_FMT("Game Room ID: {}\n", recvPkt.room_id());
	if (recvPkt.room_state() == FB_ENUMS::ROOM_STATE_TYPE_WATING)
	{
		DEBUG_LOG_FMT("State Type: WAITING\n");
	}
	else if (recvPkt.room_state() == FB_ENUMS::ROOM_STATE_TYPE_PLAYING)
	{
		DEBUG_LOG_FMT("State Type: PLAYING\n");
	}
	return true;
}
#pragma endregion

bool NetBridge::S2C::Handle_SC_LOCAL_PLAYER_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_LOCAL_PLAYER_PACKET& recvPkt
)
{

	// TODO: stanceType 사용하기

	// 로컬 플레이어 오브젝트 생성
	DEBUG_LOG_FMT("[SC_LOCAL_PLAYER_PACKET] \n");

	auto device = GLOBAL(DxDeviceGlobal).GetDevice();
	auto scene = GLOBAL(SceneGlobal).GetActiveScene();

	const uint32 id = recvPkt.player_id();
	const uint32 localID = scene->GetLocalID();
	if (id != localID)
	{
		DEBUG_LOG_FMT("is not LocalPlayer, id:{}, LocalID: {}\n", id, localID);
		return false;
	}
	DEBUG_LOG_FMT("Created LocalPlayer: {} \n", id);

	auto playerObjHandle = scene->ReserveGameObject(
		"LocalPlayer", id,
		[scene, stance = recvPkt.stance_type(), teamType = recvPkt.team_type(),  maxHP = recvPkt.max_hp(),
		 currentHP = recvPkt.current_hp(), maxStamina = recvPkt.max_stamina(), currentStamina = recvPkt.current_stamina()](GameObject* playerObj)
		{
			auto playerObjHandle = playerObj->GetHandle();

			scene->CreateComponentWithInit<MeshComponent>(
				playerObjHandle,
				[](MeshComponent* mesh)
				{
					auto meshRes = GLOBAL(ResourceGlobal).Load<MeshResource>("Resource/Models/Sphere.evmesh");
					if (nullptr != meshRes)
					{
						mesh->SetMeshResource(meshRes);
					}
				}
			);

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
				[maxHP, currentHP](HealthComponent* health) {
					health->SetMaxHealth(maxHP);
					health->SetHealth(currentHP);
				}
			);

			scene->CreateComponentWithInit<StaminaComponent>(
				playerObjHandle,
				[maxStamina, currentStamina](StaminaComponent* stamina) {
					stamina->SetMaxStamina(maxStamina);
					stamina->SetStamina(currentStamina);
				}
			);

			scene->CreateComponentWithInit<TeamComponent>(
				playerObjHandle,
				[teamType](TeamComponent* team) {
					team->SetTeamType(teamType); 
				}
			);

			scene->CreateComponentWithInit<VitalUIControllerComponent>(
				playerObjHandle,
				[](VitalUIControllerComponent* vital) {}
			);

			// FSMComponent
			scene->CreateComponentWithInit<FSMComponent>(
				playerObjHandle,
				[](FSMComponent* fsm) {
					fsm->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_IDLE);
				}
			);

			//// 공격 범위 디버깅용
			//scene->ReserveGameObject(
			//	"AttackRangeIndicator", std::nullopt,
			//	[scene, playerObjHandle](GameObject* indicatorObj)
			//	{	// 부모 설정
			//		if (auto* player = scene->TryGetGameObject(playerObjHandle)) {
			//			indicatorObj->GetTransform().SetParent(player->GetComponentHandle<Transform>());
			//		}
			//		else {
			//			scene->DestroyGameObject(indicatorObj->GetHandle());
			//			return;
			//		}

			//		// 위치
			//		indicatorObj->GetTransform().SetPosition(0.0f, -0.5f, 0.0f);

			//		// 부채꼴 Mesh
			//		auto [vertices, indices] = Resources::Sector::CreateSectorMesh(3.0f, 90.0f);
			//		scene->CreateComponentWithInit<MeshComponent>(
			//			indicatorObj->GetHandle(),
			//			[v = std::move(vertices), i = std::move(indices)](MeshComponent* mesh) {
			//				mesh->SetMesh(v, i);
			//			}
			//		);
			//	}
			//);
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
						cam->SetFollowOffsetLocal(DX::XMFLOAT3{1.0f, 1.0f, -5.0f});
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

	const uint32 id = recvPkt.obj_id();
	const uint32 localID = scene->GetLocalID();

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
		objectName = "RemotePlayer_" + std::to_string(id);
		break;
	case FB_ENUMS::GAME_OBJECT_TYPE_GENERAL:
		objectName = "Bot_" + std::to_string(id);
		break;
	default:
		objectName = "GameObject_" + std::to_string(id);
		break;
	}

	auto objectHandle = scene->ReserveGameObject(
		objectName, id,
		[scene, pos, rot, objType, teamType, maxHP = recvPkt.max_hp(),
		 currentHP = recvPkt.current_hp(), maxStamina = recvPkt.max_stamina(), currentStamina = recvPkt.current_stamina(), stance = recvPkt.stance_type()](GameObject* obj)
		{
			auto& tr = obj->GetTransform();
			tr.SetPosition(pos.x, pos.y, pos.z);
			tr.SetRotation(rot.x, rot.y, rot.z);

			auto objHandle = obj->GetHandle();

			// MeshComponent 추가
			scene->CreateComponentWithInit<MeshComponent>(
				objHandle,
				[teamType](MeshComponent* mesh)
				{
					auto meshRes = GLOBAL(ResourceGlobal).Load<MeshResource>("Resource/Models/Sphere.evmesh");
					if (nullptr != meshRes)
					{
						mesh->SetMeshResource(meshRes);
					}

					// TODO: 팀에 따라 색상 설정
				}
			);

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
				objHandle,
				[teamType](TeamComponent* team) {
					team->SetTeamType(teamType);
				}
			);

			bool isPlayer = (objType == FB_ENUMS::GAME_OBJECT_TYPE_PLAYER);

			// StaminaComponent (Player Only)
			if (isPlayer)
			{
				scene->CreateComponentWithInit<StaminaComponent>(
					objHandle,
					[maxStamina, currentStamina](StaminaComponent* stamina) {
						stamina->SetMaxStamina(maxStamina);
						stamina->SetStamina(currentStamina);
					}
				);
			}

			// VitalUIControllerComponent
			if (isPlayer || objType == FB_ENUMS::GAME_OBJECT_TYPE_SOLDIER || objType == FB_ENUMS::GAME_OBJECT_TYPE_GENERAL)
			{
				scene->CreateComponentWithInit<VitalUIControllerComponent>(
					objHandle,
					[](VitalUIControllerComponent* vital) {
						// Init 제거->OnStart에서 자동 판단
					}
				);
			}

			DEBUG_LOG_FMT(
				"Created {} at ({:.2f}, {:.2f}, {:.2f}), HP: {}/{}\n",
				objType == FB_ENUMS::GAME_OBJECT_TYPE_PLAYER ? "Player" : "Bot", pos.x, pos.y, pos.z, currentHP, maxHP
			);

			// BattleUIControllerComponent 부착
			if (objType == FB_ENUMS::GAME_OBJECT_TYPE_PLAYER)
			{
				scene->CreateComponentWithInit<BattleUIControllerComponent>(
					objHandle,
					[stance](BattleUIControllerComponent* ui)
					{
						ui->SetControlMode(BattleUIControllerComponent::ControlType::Remote);
						ui->InitStance(stance);
						//DEBUG_LOG_FMT("[BattleUI] Component attached to RemotePlayer. InitStance: {}\n", static_cast<int>(stance));
					}
				);
			}

	//		// 공격 범위 디버깅
	//		if (objType == FB_ENUMS::GAME_OBJECT_TYPE_PLAYER || objType == FB_ENUMS::GAME_OBJECT_TYPE_GENERAL)
	//		{
	//			scene->ReserveGameObject(
	//				"AttackRangeIndicator", std::nullopt,
	//				[scene, objHandle](GameObject* indicatorObj)
	//				{
	//					// 부모
	//					if (auto* parent = scene->TryGetGameObject(objHandle)) {
	//						indicatorObj->GetTransform().SetParent(parent->GetComponentHandle<Transform>());
	//					}

	//					// 위치
	//					indicatorObj->GetTransform().SetPosition(0.0f, -0.5f, 0.0f);

	//					// 부채꼴 Mesh
	//					auto [vertices, indices] = Resources::Sector::CreateSectorMesh(3.0f, 90.0f);
	//					scene->CreateComponentWithInit<MeshComponent>(
	//						indicatorObj->GetHandle(),
	//						[v = std::move(vertices), i = std::move(indices)](MeshComponent* mesh) {
	//							mesh->SetMesh(v, i);
	//						}
	//					);
	//				}
	//			);
	//		}
		}
	);

	return true;
}

bool NetBridge::S2C::Handle_SC_REMOVE_OBJ_PACKET(const SOCKET& socket, const FB_TABLES::SC_REMOVE_OBJ_PACKET& recvPkt)
{
	// 원격 플레이어 또는 봇 오브젝트 제거
	// TODO: 원격 플레이어 또는 봇 오브젝트를 게임 씬에서 제거하기
	auto scene = GLOBAL(SceneGlobal).GetActiveScene();

	const uint32 id = recvPkt.obj_id();
	const uint32 localID = scene->GetLocalID();

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
	// 원격 플레이어 또는 봇 오브젝트 이동 처리
	// TODO: 원격 플레이어 또는 봇 오브젝트의 위치 및 회전을 업데이트하기
	auto scene = GLOBAL(SceneGlobal).GetActiveScene();

	const uint32 id = recvPkt.obj_id();
	const uint32 localID = scene->GetLocalID();

	if (id == localID)
	{
		return false;
	}

	auto obj = scene->FindGameObjectByServerID(id);
	if (obj)
	{
		const Vec3 pos{recvPkt.pos_info()->pos().x(), recvPkt.pos_info()->pos().y(), recvPkt.pos_info()->pos().z()};
		const Vec3 rot{recvPkt.pos_info()->rot().x(), recvPkt.pos_info()->rot().y(), recvPkt.pos_info()->rot().z()};
		obj->GetTransform().SetPosition(pos);
		obj->GetTransform().SetRotation(rot); // TODO: degree인지 quaternion인지 확인 필요
	}
	return true;
}

bool NetBridge::S2C::Handle_SC_PLAYER_ATTACK_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_PLAYER_ATTACK_PACKET& recvPkt
)
{
	auto scene = GLOBAL(SceneGlobal).GetActiveScene();
	const uint32 id = recvPkt.obj_id();
	const uint32 localID = scene->GetLocalID();

	// 서버 Echo 방지 (로컬 플레이어는 이미 입력 시점에 처리)
	if (id == localID)
	{
		return true;
	}

	const auto attackInfo = recvPkt.attack_info();
	if (!attackInfo) return false;

	const auto type = attackInfo->attack_type();
	const auto dir = attackInfo->attack_dir();

	// 1. 공격자 오브젝트 찾기
	if (auto* obj = scene->FindGameObjectByServerID(id))
	{
		// 2. 컴포넌트 가져오기
		if (auto* uiController = obj->GetComponent<BattleUIControllerComponent>())
		{
			// 3. UI 갱신
			uiController->TriggerAttackRemote(type, dir);
			// DEBUG_LOG_FMT("[SC_PLAYER_ATTACK] ID: {}, Type: {}, Dir: {}\n", id, static_cast<int>(type), static_cast<int>(dir));
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

	const uint32 objID = recvPkt.obj_id();
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


		std::cout << std::format("ID:{}, hp:{}, stanmina:{}", objID, hp, stamina);

		auto		 staminaComp = obj->GetComponent<StaminaComponent>();
		if (staminaComp)
		{
			staminaComp->SetStamina(stamina);
		}
	}

	return true;
}

bool NetBridge::S2C::Handle_SC_CHANGE_PLAYER_STANCE_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_CHANGE_PLAYER_STANCE_PACKET& recvPkt
)
{
	auto scene = GLOBAL(SceneGlobal).GetActiveScene();
	const uint32 id = recvPkt.obj_id();
	const auto stance = recvPkt.stance_type();

	//서버 Echo 방지
	if (id == scene->GetLocalID())
	{
		return true;
	}

	if (auto* obj = scene->FindGameObjectByServerID(id))
	{
		if (auto* ui = obj->GetComponent<BattleUIControllerComponent>())
		{
			ui->SetStance(stance);
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
	if (!scene) return false;

	const uint32 objID = recvPkt.obj_id();
	auto obj = scene->FindGameObjectByServerID(objID);
	if (!obj)
	{
		return false;
	}

	uint8_t nextState = recvPkt.next_state();

	// FSM 상태 동기화
	if (auto* fsm = obj->GetComponent<FSMComponent>())
	{
		fsm->ChangeState(nextState);
	}

	// 사망 시 조건부 삭제 (StaminaComponent 유무로 장수 여부 판별)
	if (nextState == FB_ENUMS::GENERAL_STATE_TYPE_DEAD)
	{
		if (obj->GetComponent<StaminaComponent>() == nullptr)
		{
			// 병사는 즉시 메모리에서 삭제
			scene->DestroyGameObject(obj->GetHandle());
		}
		// 장수(플레이어)는 삭제하지 않음
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
	const uint32_t totalSeconds = remainingTime / 1000;
	const uint32_t minutes = totalSeconds / 60;
	const uint32_t seconds = totalSeconds % 60;
	//DEBUG_LOG_FMT("Remaining Time: {:02d}M:{:02d}S\n", minutes, seconds);
	return true;
}

bool NetBridge::S2C::Handle_SC_CHANGE_CAMERA_TARGET_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_CHANGE_CAMERA_TARGET_PACKET& recvPkt
)
{
	// 카메라 타겟 변경
	auto scene = GLOBAL(SceneGlobal).GetActiveScene();
	auto cameraComp = CameraComponent::GetMainCamera();

	if (!cameraComp) return false;

	const uint32 cameraTargetID = recvPkt.camera_target_id();
	
	if (cameraTargetID == 0)
	{
		// 락온 해제 시(적 죽었을 때) 로컬 플레이어로 복구
		const uint32 localID = scene->GetLocalID();
		if (auto localPlayer = scene->FindGameObjectByServerID(localID))
		{
			cameraComp->SetLookAtTarget(localPlayer->GetHandle());
			cameraComp->SetEnableLookAtRotation(false); // 자유 시점
			cameraComp->SetFollowOffsetLocal({1.0f, 1.0f, -5.0f}); // 오프셋 복구
			DEBUG_LOG_FMT("[SC_CHANGE_CAMERA_TARGET_PACKET] Camera Reset to LocalPlayer\n");
		}
		else
		{
			cameraComp->ClearLookAtTarget();
			DEBUG_LOG_FMT("[SC_CHANGE_CAMERA_TARGET_PACKET] Camera Target Cleared (LocalPlayer not found)\n");
		}
	}
	else
	{
		if (auto targetObj = scene->FindGameObjectByServerID(cameraTargetID))
		{
			cameraComp->SetLookAtTarget(targetObj->GetHandle());
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

bool NetBridge::S2C::Handle_SC_SHOW_PLAYER_ATTACK_DIR_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_SHOW_PLAYER_ATTACK_DIR_PACKET& recvPkt
)
{
	// 플레이어 공격 방향 표시
	auto scene = GLOBAL(SceneGlobal).GetActiveScene();
	
	const uint32 id = recvPkt.player_id();
	const uint32 localID = scene->GetLocalID();

	// 서버 Echo 방지
	if (id == localID)
	{
		return true;
	}

	const uint8_t dirRaw = recvPkt.attack_dir();
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
	auto scene = GLOBAL(SceneGlobal).GetActiveScene();
	const uint32 objID = recvPkt.obj_id();
	auto obj = scene->FindGameObjectByServerID(objID);

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
		if (auto* ui = obj->GetComponent<BattleUIControllerComponent>())
		{
			ui->SetStance(recvPkt.stance_type());
		}
		//디버깅
		DEBUG_LOG_FMT("[SC_RESPAWN_GENERAL_PACKET] Object ID {} has respawned at ({:.1f}, {:.1f}, {:.1f})\n", 
			objID, pos.x, pos.y, pos.z);
		
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