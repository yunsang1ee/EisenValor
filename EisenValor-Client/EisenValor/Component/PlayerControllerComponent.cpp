#include "stdafxClient.h"
#include "PlayerControllerComponent.h"
#include "GameObject.h"
#include "InputGlobal.h"
#include "Transform.h"
#include "SceneGlobal.h"
#include "Scene.h"
#include "Component/FSM/FSMComponent.h"
#include "AnimationComponent.h"
#include "DxMath.h"

#include "Util/CameraConfig.h"

#include "NetworkGlobal.h"
#include "Packets/C2SPackets.h"
#include "Packets/Enums_generated.h"
#include "Packets/Structs_generated.h"

using namespace DirectX;
using namespace FB_ENUMS;

namespace
{
constexpr float kMinMouseDelta = 1e-4f;
}

void PlayerControllerComponent::SetMouseSensitivity(float x, float y)
{
	m_sensitivityX = x;
	m_sensitivityY = y;
}

void PlayerControllerComponent::SetPitchLimits(float minPitch, float maxPitch)
{
	m_minPitch = minPitch;
	m_maxPitch = maxPitch;
}

void PlayerControllerComponent::OnStart()
{
	auto* myGameObject = GetGameObject();
	if (!myGameObject)
	{
		return;
	}

	m_movementHandle = myGameObject->GetComponentHandle<MovementComponent>();

	auto camHandle = myGameObject->GetComponentHandle<CameraComponent>();
	if (camHandle.IsValid())
	{
		m_cameraObjectHandle = myGameObject->GetHandle();
	}
	else
	{
		FindCameraInChildren(myGameObject);
	}

	InitializePitchFromCamera();

	auto& input = GLOBAL(InputGlobal);
	input.SetMouseLocked(true);

	DEBUG_LOG_FMT("[PlayerControllerComponent] Mouse locked for gameplay\n");
}

void PlayerControllerComponent::OnDestroy()
{
	auto& input = GLOBAL(InputGlobal);
	input.SetMouseLocked(false);

	DEBUG_LOG_FMT("[PlayerControllerComponent] Mouse unlocked\n");
}

void PlayerControllerComponent::OnUpdate(float deltaTime)
{
	auto& input = GLOBAL(InputGlobal);
	if (input.GetInputDown(VK_F1))
	{
		input.ToggleMouseLock();
	}

	auto* myGameObject = GetGameObject();
	if (!myGameObject)
	{
		return;
	}

	auto* fsm = myGameObject->GetComponent<FSMComponent>();
	if (!fsm)
		return;

	auto curState = fsm->GetCurStateType();
	if (curState == FB_ENUMS::PLAYER_STATE_TYPE_DEAD)
	{
		return;
	}

	// Root Motion Delta
	if (auto* animComp = myGameObject->GetComponent<AnimationComponent>())
	{
		XMVECTOR rootDelta = animComp->ConsumeRootMotionDelta();

		// 델타가 유효하다면 이동 적용
		if (XMVector3LengthSq(rootDelta).m128_f32[0] > 1e-6f)
		{
			auto&	 transform = myGameObject->GetTransform();
			XMFLOAT3 currentWorldPos = transform.GetWorldPosition();
			XMVECTOR newWorldPos = XMVectorAdd(XMLoadFloat3(&currentWorldPos), rootDelta);

			XMFLOAT3 finalPos;
			XMStoreFloat3(&finalPos, newWorldPos);
			transform.SetWorldPosition(finalPos);

			// 이동 후 서버 패킷 보내기
			auto				rot = transform.GetRotation();
			FB_STRUCTS::PosInfo posInfo{{finalPos.x, finalPos.y, finalPos.z}, {rot.x, rot.y, rot.z}};
			auto				pbMove = NetBridge::C2S::Make_CS_MOVE_PACKET(&posInfo, fsm->GetMoveDirection());
			GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pbMove));

			// DEBUG_LOG_FMT("[PlayerController] RootMotion Applied. NewPos:({:.2f}, {:.2f}, {:.2f})\n",
			//	finalPos.x, finalPos.y, finalPos.z);
		}
	}

	// 락온 상태 시 숄더뷰 갱신
	if (m_cameraObjectHandle.IsValid())
	{
		auto* scene = GLOBAL(SceneGlobal).GetActiveScene();
		if (scene)
		{
			if (auto* camObj = scene->TryGetGameObject(m_cameraObjectHandle))
			{
				if (auto* camComp = camObj->GetComponent<CameraComponent>())
				{
					if (camComp->IsLookAtRotationEnabled())
					{
						// 1. 플레이어를 먼저 적 방향으로 회전
						RotatePlayerToTarget(camComp);

						// 플레이어의 위치를 기준으로 숄더뷰 위치 업데이트
						UpdateCameraShoulderView(camComp);
					}
				}
			}
		}
	}

	ProcessMouseRotation(deltaTime);
	ProcessMovementInput(deltaTime);
}

void PlayerControllerComponent::OnFixedUpdate(float deltaTime)
{
	auto* myGameObject = GetGameObject();
	if (!myGameObject)
	{
		return;
	}

	auto& transform = myGameObject->GetTransform();
	auto  pos = transform.GetPosition();
	auto  rot = transform.GetRotation();

	FB_STRUCTS::Vec3	posVec{pos.x, pos.y, pos.z};
	FB_STRUCTS::Vec3	rotVec{rot.x, rot.y, rot.z};
	FB_STRUCTS::PosInfo posInfo{posVec, rotVec};
}

void PlayerControllerComponent::ProcessMouseRotation(float deltaTime)
{
	auto& input = GLOBAL(InputGlobal);

	if (!input.IsMouseLocked() || !input.IsWindowFocused())
	{
		return;
	}

	// 카메라 및 락온 상태 확인
	if (!m_cameraObjectHandle.IsValid())
		return;
	auto* scene = GLOBAL(SceneGlobal).GetActiveScene();
	if (!scene)
		return;
	auto* camObj = scene->TryGetGameObject(m_cameraObjectHandle);
	if (!camObj)
		return;
	auto* camComp = camObj->GetComponent<CameraComponent>();
	if (!camComp)
		return;

	bool isLookAtLocked = camComp->IsLookAtRotationEnabled();

	// 락온 상태일 때 처리
	if (isLookAtLocked)
	{
		// (락온 시) 카메라 오프셋을 0으로 보간하여 적을 정면으로 보게 함
		float lerpFactor = 1.0f - expf(-10.0f * deltaTime);
		XMFLOAT3 currentOffset = camComp->GetLookAtRotationOffset();
		currentOffset.x = std::lerp(currentOffset.x, 0.0f, lerpFactor);
		currentOffset.y = std::lerp(currentOffset.y, 0.0f, lerpFactor);
		camComp->SetLookAtRotationOffset(currentOffset);

		// (락온 해제 시) 카메라가 플레이어 뒤를 비추도록 m_yaw 동기화
		auto&	 transform = GetGameObject()->GetTransform();
		XMFLOAT3 fwd = transform.GetForward();
		m_yaw = XMConvertToDegrees(atan2f(fwd.x, fwd.z));
		m_pitch = currentOffset.x;
		return;
	}

	const auto& mouseState = input.GetMouseDelta();
	const float deltaX = mouseState.x * m_sensitivityX;
	const float deltaY = mouseState.y * m_sensitivityY;

	bool isMouseMoved = (fabsf(deltaX) > kMinMouseDelta || fabsf(deltaY) > kMinMouseDelta);

	if (fabsf(deltaX) > kMinMouseDelta)
	{
		RotateYaw(deltaX);
	}

	if (fabsf(deltaY) > kMinMouseDelta)
	{
		RotatePitch(deltaY);
	}

	// 비락온 상태에서만 직접 계산한 m_yaw, m_pitch와 월드 오프셋을 적용
	// 회전 적용
	camComp->SetLookAtRotationOffset({m_pitch, m_yaw, 0.0f});

	// 월드 기준 오프셋 계산 (m_yaw, m_pitch 기반)
	XMMATRIX rotationMatrix =
		XMMatrixRotationRollPitchYaw(XMConvertToRadians(m_pitch), XMConvertToRadians(m_yaw), 0.0f);

	XMVECTOR baseOffset = XMVectorSet(
		CameraConfig::kDefaultLocalOffsetX, CameraConfig::kCameraHeight, CameraConfig::kDefaultLocalOffsetZ, 0.0f
	);
	XMVECTOR worldOffset = XMVector3TransformNormal(baseOffset, rotationMatrix);

	XMFLOAT3 finalOffset;
	XMStoreFloat3(&finalOffset, worldOffset);

	// 비락온 시 월드 오프셋 적용 (SetFollowOffset 사용)
	camComp->SetFollowOffset(finalOffset);

		auto* myGameObject = GetGameObject();
	if (!myGameObject)
	{
		return;
	}

	auto* fsm = myGameObject->GetComponent<FSMComponent>();
	if (!fsm)
		return;

	// 마우스가 움직였다면 서버에 현재 위치/회전 패킷 전송 (연결 유지 및 동기화)
	if (isMouseMoved)
	{
		auto* myGameObject = GetGameObject();
		if (myGameObject)
		{
			auto&				transform = myGameObject->GetTransform();
			auto				pos = transform.GetPosition();
			auto				rot = transform.GetRotation();
			FB_STRUCTS::PosInfo posInfo{{pos.x, pos.y, pos.z}, {rot.x, rot.y, rot.z}};
			auto				pb = NetBridge::C2S::Make_CS_MOVE_PACKET(&posInfo, fsm->GetMoveDirection());
			GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
		}
	}
}

void PlayerControllerComponent::ProcessMovementInput(float deltaTime)
{
	if (!m_movementHandle.IsValid())
		return;

	auto* myGameObject = GetGameObject();
	if (!myGameObject)
		return;

	auto* movement = myGameObject->GetComponent<MovementComponent>();
	auto* fsm = myGameObject->GetComponent<FSMComponent>();

	if (!movement || !fsm)
		return;

	uint8_t curState = fsm->GetCurStateType();
	if (curState == FB_ENUMS::PLAYER_STATE_TYPE_DEAD)
		return;

	auto& input = GLOBAL(InputGlobal);

	// 1. 전투 태세 전환 (Ctrl)
	if (input.GetInputDown(VK_CONTROL))
	{
		if (fsm->GetStance() == FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL)
		{
			fsm->SetStance(static_cast<uint8_t>(GENERAL_STANCE_TYPE_COMBAT));
			DEBUG_LOG_FMT("[PlayerController] Switch to COMBAT\n");
		}
		else
		{
			fsm->SetStance(static_cast<uint8_t>(GENERAL_STANCE_TYPE_NEUTRAL));

			// 카메라 및 상태 복구
			if (auto* mainCamera = CameraComponent::GetMainCamera())
			{
				mainCamera->SetLookAtTarget(myGameObject->GetComponentHandle<Transform>());
				mainCamera->SetEnableLookAtRotation(false);
				mainCamera->SetFollowOffsetLocal(
					{CameraConfig::kDefaultLocalOffsetX, CameraConfig::kCameraHeight, CameraConfig::kDefaultLocalOffsetZ
					}
				);
				DEBUG_LOG_FMT("[PlayerController] Switch to NEUTRAL & Reset Camera\n");
			}
		}
		// 서버 알림
		auto pb = NetBridge::C2S::Make_CS_CHANGE_GENERAL_STANCE_PACKET();
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}

	// 2. 중립 상태 공격 (LBUTTON / RBUTTON)
	if (fsm->GetStance() == FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL)
	{
		// 약공격
		if (input.GetInputDown(VK_LBUTTON))
		{
			// RequestState에서 공격 가능 여부 판단
			if (fsm->RequestState(FSMComponent::StateRequestType::AttackLight))
			{
				FB_STRUCTS::GeneralAttackInfo attackInfo(GENERAL_ATTACK_TYPE_LIGHT, GENERAL_ATTACK_DIR_TYPE_NONE);
				auto						  pb = NetBridge::C2S::Make_CS_GENERAL_ATTACK_PACKET(&attackInfo);
				GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));

				fsm->SetCurAttackType(static_cast<uint8_t>(GENERAL_ATTACK_TYPE_LIGHT));
				DEBUG_LOG_FMT("[PlayerController] Neutral Quick Attack!\n");
			}
			return;
		}
		// 강공격
		else if (input.GetInputDown(VK_RBUTTON))
		{
			if (fsm->RequestState(FSMComponent::StateRequestType::AttackHeavy))
			{
				FB_STRUCTS::GeneralAttackInfo attackInfo(GENERAL_ATTACK_TYPE_HEAVY, GENERAL_ATTACK_DIR_TYPE_NONE);
				auto						  pb = NetBridge::C2S::Make_CS_GENERAL_ATTACK_PACKET(&attackInfo);
				GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));

				fsm->SetCurAttackType(static_cast<uint8_t>(GENERAL_ATTACK_TYPE_HEAVY));
				DEBUG_LOG_FMT("[PlayerController] Neutral Heavy Attack!\n");
			}
			return;
		}
	}

	bool w = input.GetInput('W');
	bool s = input.GetInput('S');
	bool a = input.GetInput('A');
	bool d = input.GetInput('D');
	bool isShiftPressed = input.GetInput(VK_SHIFT);

	bool isMovingInput = (w || s || a || d);

	// Run
	bool isNeutralStance = (fsm->GetStance() == FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL);
	bool isRunning = isShiftPressed && isMovingInput && isNeutralStance;

	if (isRunning)
	{
		movement->SetMoveSpeed(8.0f);
	}
	else
	{
		movement->SetMoveSpeed(4.5f);
	}

	// 락온 상태 & 카메라 방향 업데이트
	bool	 isLockOn = false;
	XMVECTOR camFwd = XMVectorSet(0, 0, 1, 0);

	if (m_cameraObjectHandle.IsValid())
	{
		auto* scene = GLOBAL(SceneGlobal).GetActiveScene();
		if (scene)
		{
			if (auto* camObj = scene->TryGetGameObject(m_cameraObjectHandle))
			{
				if (auto* camComp = camObj->GetComponent<CameraComponent>())
				{
					isLockOn = camComp->IsLookAtRotationEnabled();
					XMFLOAT3 f = camComp->GetGameObject()->GetTransform().GetForward();
					camFwd = XMLoadFloat3(&f);
				}
			}
		}
	}
	fsm->SetLockOn(isLockOn);

	// 이동 방향 및 캐릭터 회전 설정
	if (isLockOn)
	{
		if (w)
			fsm->SetMoveDirection(FB_ENUMS::MOVE_DIRECTION_TYPE_FWD);
		else if (s)
			fsm->SetMoveDirection(FB_ENUMS::MOVE_DIRECTION_TYPE_BWD);
		else if (a)
			fsm->SetMoveDirection(FB_ENUMS::MOVE_DIRECTION_TYPE_LFT);
		else if (d)
			fsm->SetMoveDirection(FB_ENUMS::MOVE_DIRECTION_TYPE_RGT);

		movement->SetInputForward(w);
		movement->SetInputBackward(s);
		movement->SetInputLeft(a);
		movement->SetInputRight(d);

		movement->SetMoveSpeed(1.5f);
	}
	else
	{
		fsm->SetMoveDirection(FB_ENUMS::MOVE_DIRECTION_TYPE_FWD);

		if (isMovingInput)
		{
			// 카메라 기준 방향 계산
			XMVECTOR baseFwd = XMVector3Normalize(XMVectorSetY(camFwd, 0.0f));
			if (XMVector3LengthSq(baseFwd).m128_f32[0] < 1e-6f)
				baseFwd = XMVectorSet(0, 0, 1, 0);

			XMVECTOR baseRight = XMVector3Normalize(XMVector3Cross(XMVectorSet(0, 1, 0, 0), baseFwd));

			XMVECTOR moveDir = XMVectorZero();
			if (w)
				moveDir = XMVectorAdd(moveDir, baseFwd);
			if (s)
				moveDir = XMVectorSubtract(moveDir, baseFwd);
			if (a)
				moveDir = XMVectorSubtract(moveDir, baseRight);
			if (d)
				moveDir = XMVectorAdd(moveDir, baseRight);

			moveDir = XMVector3Normalize(moveDir);

			// 캐릭터 몸 회전 (Slerp)
			float	 targetYaw = atan2f(XMVectorGetX(moveDir), XMVectorGetZ(moveDir));
			XMVECTOR targetRotQ = XMQuaternionRotationAxis(XMVectorSet(0, 1, 0, 0), targetYaw);

			auto&	 tr = myGameObject->GetTransform();
			auto	 q = tr.GetRotationQuaternion();
			XMVECTOR curRotQ = XMLoadFloat4(&q);
			XMVECTOR nextRotQ = XMQuaternionSlerp(curRotQ, targetRotQ, 0.2f);

			XMFLOAT4 nextRotF;
			XMStoreFloat4(&nextRotF, nextRotQ);
			tr.SetRotationQuaternion(nextRotF);

			movement->SetInputForward(true);
		}
		else
		{
			movement->SetInputForward(false);
		}
		movement->SetInputBackward(false);
		movement->SetInputLeft(false);
		movement->SetInputRight(false);
	}

	bool hasJustReleased =
		input.GetInputUp('W') || input.GetInputUp('A') || input.GetInputUp('S') || input.GetInputUp('D');

	bool hasJustPressed =
		input.GetInputDown('W') || input.GetInputDown('A') || input.GetInputDown('S') || input.GetInputDown('D');

	bool isRestricted =
		(curState == FB_ENUMS::PLAYER_STATE_TYPE_PRE_DELAY || curState == FB_ENUMS::PLAYER_STATE_TYPE_ATTACK ||
		 curState == FB_ENUMS::PLAYER_STATE_TYPE_POST_DELAY);

	auto&				transform = myGameObject->GetTransform();
	auto				pos = transform.GetPosition();
	auto				rot = transform.GetRotation();
	FB_STRUCTS::PosInfo posInfo{{pos.x, pos.y, pos.z}, {rot.x, rot.y, rot.z}};

	if (isRestricted)
	{
		if (isMovingInput)
		{
			auto stateType = isRunning ? FB_ENUMS::PLAYER_STATE_TYPE_RUN : FB_ENUMS::PLAYER_STATE_TYPE_WALK;

			auto pbState = NetBridge::C2S::Make_CS_UPDATE_PLAYER_STATE_PACKET(stateType);
			GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pbState));
		}
		auto pbMove = NetBridge::C2S::Make_CS_MOVE_PACKET(&posInfo, fsm->GetMoveDirection());
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pbMove));
		return;
	}

	if (isMovingInput)
	{
		if (isRunning)
		{
			if (curState != FB_ENUMS::PLAYER_STATE_TYPE_RUN || hasJustPressed)
			{
				fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_RUN);

				auto pbState = NetBridge::C2S::Make_CS_UPDATE_PLAYER_STATE_PACKET(FB_ENUMS::PLAYER_STATE_TYPE_RUN);
				GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pbState));
			}
		}
		else
		{
			if (curState != FB_ENUMS::PLAYER_STATE_TYPE_WALK || hasJustPressed)
			{
				fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_WALK);

				auto pbState = NetBridge::C2S::Make_CS_UPDATE_PLAYER_STATE_PACKET(FB_ENUMS::PLAYER_STATE_TYPE_WALK);
				GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pbState));
			}
		}

		auto pbMove = NetBridge::C2S::Make_CS_MOVE_PACKET(&posInfo, fsm->GetMoveDirection());
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pbMove));
	}
	else
	{
		if (curState == FB_ENUMS::PLAYER_STATE_TYPE_WALK || curState == FB_ENUMS::PLAYER_STATE_TYPE_RUN ||
			hasJustReleased)
		{
			fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_IDLE);

			auto pbState = NetBridge::C2S::Make_CS_UPDATE_PLAYER_STATE_PACKET(FB_ENUMS::PLAYER_STATE_TYPE_IDLE);
			GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pbState));

			auto pbMove = NetBridge::C2S::Make_CS_MOVE_PACKET(&posInfo, fsm->GetMoveDirection());
			GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pbMove));
		}
	}

	if (input.GetInputDown('F'))
	{
		auto pb = NetBridge::C2S::Make_CS_GEN_NPC_GENREAL_PACKET();
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}

	if (input.GetInputDown('G'))
	{
		auto pb = NetBridge::C2S::Make_CS_GEN_NPC_SOLDIER_PACKET();
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}

	if (input.GetInputDown('1'))
	{
		auto pb{NetBridge::C2S::Make_CS_TELEPORT_PACKET(FB_ENUMS::TELEPORT_PLACE_TYPE_MY_TEAM_BASE)};
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}

	if (input.GetInputDown('2'))
	{
		auto pb{NetBridge::C2S::Make_CS_TELEPORT_PACKET(FB_ENUMS::TELEPORT_PLACE_TYPE_OPPONENT_TEAM_BASE)};
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}

	if (input.GetInputDown('3'))
	{
		auto pb{NetBridge::C2S::Make_CS_TELEPORT_PACKET(FB_ENUMS::TELEPORT_PLACE_TYPE_OCCUPATION_ZONE_A)};
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}

	if (input.GetInputDown('4'))
	{
		auto pb{NetBridge::C2S::Make_CS_TELEPORT_PACKET(FB_ENUMS::TELEPORT_PLACE_TYPE_OCCUPATION_ZONE_B)};
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}

	if (input.GetInputDown('5'))
	{
		auto pb{NetBridge::C2S::Make_CS_TELEPORT_PACKET(FB_ENUMS::TELEPORT_PLACE_TYPE_HEAL_ZONE)};
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}

	if (input.GetInputDown('E'))
	{
		auto pb{NetBridge::C2S::Make_CS_PLAYER_FAKE_PACKET()};
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}


}

void PlayerControllerComponent::RotateYaw(float deltaDegrees)
{
	m_yaw += deltaDegrees;
	if (m_yaw > 360.0f)
		m_yaw -= 360.0f;
	if (m_yaw < 0.0f)
		m_yaw += 360.0f;
}

void PlayerControllerComponent::RotatePitch(float deltaDegrees)
{
	m_pitch += deltaDegrees;
	m_pitch = std::clamp(m_pitch, m_minPitch, m_maxPitch);
}

void PlayerControllerComponent::FindCameraInChildren(GameObject* parentGameObject)
{
	auto& transform = parentGameObject->GetTransform();
	auto* scene = GLOBAL(SceneGlobal).GetActiveScene();
	if (!scene)
	{
		return;
	}

	auto* transformStorage = scene->GetStorage<Transform>();
	if (!transformStorage)
	{
		return;
	}

	for (const auto& childHandle : transform.GetChildren())
	{
		auto* childTransform = transformStorage->Get(childHandle);
		if (!childTransform)
		{
			continue;
		}

		auto  childObjHandle = childTransform->GetOwner();
		auto* childGO = scene->TryGetGameObject(childObjHandle);
		if (!childGO)
		{
			continue;
		}

		if (childGO->GetComponentHandle<CameraComponent>().IsValid())
		{
			m_cameraObjectHandle = childObjHandle;
			break;
		}
	}
}

void PlayerControllerComponent::InitializePitchFromCamera()
{
	if (!m_cameraObjectHandle.IsValid())
	{
		return;
	}

	auto* scene = GLOBAL(SceneGlobal).GetActiveScene();
	if (!scene)
	{
		return;
	}

	auto* camObj = scene->TryGetGameObject(m_cameraObjectHandle);
	if (!camObj)
	{
		return;
	}

	auto* camComp = camObj->GetComponent<CameraComponent>();
	if (!camComp)
	{
		return;
	}

	m_pitch = camComp->GetLookAtRotationOffset().x;
}

void PlayerControllerComponent::UpdateCameraShoulderView(CameraComponent* camComp)
{
	auto* scene = GLOBAL(SceneGlobal).GetActiveScene();
	if (!scene)
		return;

	auto* trStorage = scene->GetStorage<Transform>();
	auto* playerTr = GetGameObject()->GetComponent<Transform>();

	auto  lookAtHandle = camComp->GetLookAtTarget();
	auto* enemyTr = trStorage->Get(lookAtHandle);

	if (playerTr && enemyTr)
	{
		XMFLOAT3 pPos = playerTr->GetWorldPosition();
		XMVECTOR playerPos = XMLoadFloat3(&pPos);

		XMFLOAT3 ePos = enemyTr->GetWorldPosition();
		XMVECTOR enemyPos = XMLoadFloat3(&ePos);

		// RotationToTarget에서 이미 계산된 플레이어의 정면 벡터를 사용하여 숄더뷰 위치를 계산
		XMFLOAT3 playerFwdF = playerTr->GetForward();
		XMVECTOR playerFwd = XMLoadFloat3(&playerFwdF);
		XMVECTOR playerFwdH = XMVector3Normalize(XMVectorSetY(playerFwd, 0.0f));

		if (XMVector3LengthSq(playerFwdH).m128_f32[0] < 1e-6f)
		{
			playerFwdH = XMVectorSet(0, 0, 1, 0);
		}

		// 오른쪽 방향 계산 (수평)
		XMVECTOR up = XMVectorSet(0, 1, 0, 0);
		XMVECTOR rightH = XMVector3Normalize(XMVector3Cross(up, playerFwdH));

		// 숄더뷰 오프셋(오른쪽, 위, 뒤)
		// 플레이어의 현재 정면(playerFwdH)의 반대 방향으로 카메라 배치
		XMVECTOR offset = XMVectorScale(rightH, CameraConfig::kShoulderViewOffsetX) +
						  XMVectorSet(0, CameraConfig::kShoulderViewOffsetY, 0, 0) +
						  XMVectorScale(playerFwdH, CameraConfig::kShoulderViewOffsetZ);

		XMFLOAT3 offsetF;
		XMStoreFloat3(&offsetF, offset);

		camComp->SetFollowOffset(offsetF);
	}
}

void PlayerControllerComponent::RotatePlayerToTarget(CameraComponent* camComp)
{
	auto* scene = GLOBAL(SceneGlobal).GetActiveScene();
	if (!scene)
		return;

	auto* trStorage = scene->GetStorage<Transform>();
	auto* playerTr = GetGameObject()->GetComponent<Transform>();

	auto  lookAtHandle = camComp->GetLookAtTarget();
	auto* enemyTr = trStorage->Get(lookAtHandle);

	if (playerTr && enemyTr)
	{
		XMFLOAT3 pPos = playerTr->GetWorldPosition();
		XMFLOAT3 ePos = enemyTr->GetWorldPosition();

		// 적 방향 벡터 (수평)
		float dx = ePos.x - pPos.x;
		float dz = ePos.z - pPos.z;

		// 거리 체크
		if (dx * dx + dz * dz < 1e-4f)
			return;

		// 타겟 방향 Yaw 계산
		float targetYaw = atan2f(dx, dz);

		// 현재 플레이어의 Yaw 가져오기
		// forward 벡터
		XMFLOAT3 fwd = playerTr->GetForward();
		float	 currentYaw = atan2f(fwd.x, fwd.z);

		// 각도 최단 경로 보간
		float diff = targetYaw - currentYaw;
		while (diff <= -XM_PI)
			diff += XM_2PI;
		while (diff > XM_PI)
			diff -= XM_2PI;

		// 보간
		float newYaw = currentYaw + diff * 0.2f;

		// Yaw 회전
		XMVECTOR newRotQ = XMQuaternionRotationAxis(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), newYaw);

		XMFLOAT4 newRotQF;
		XMStoreFloat4(&newRotQF, newRotQ);
		playerTr->SetRotationQuaternion(newRotQF);
	}
}