#include "stdafxClient.h"
#include "PlayerControllerComponent.h"
#include "GameObject.h"
#include "InputGlobal.h"
#include "Transform.h"
#include "SceneGlobal.h"
#include "Scene.h"
#include "Component/FSM/FSMComponent.h"

#include "Util/CameraConfig.h"

#include "NetworkGlobal.h"
#include "Packets/C2SPackets.h"
#include "Packets/Structs_generated.h"

using namespace DirectX;

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
						UpdateCameraShoulderView(camComp);
						RotatePlayerToTarget(camComp);
					}
				}
			}
		}
	}

	auto* myGameObject = GetGameObject();
	if (!myGameObject)
	{
		return;
	}

	auto* fsm = myGameObject->GetComponent<FSMComponent>();
	if (!fsm)
		return;


	const auto curState{fsm->GetCurStateType()};

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

	// 카메라의 락온(LookAt Rotation) 활성화 여부 확인
	bool isLookAtLocked = false;
	if (m_cameraObjectHandle.IsValid())
	{
		auto* scene = GLOBAL(SceneGlobal).GetActiveScene();
		if (scene)
		{
			if (auto* camObj = scene->TryGetGameObject(m_cameraObjectHandle))
			{
				if (auto* camComp = camObj->GetComponent<CameraComponent>())
				{
					isLookAtLocked = camComp->IsLookAtRotationEnabled();
				}
			}
		}
	}

	// 락온 상태면 마우스 회전 무시
	if (isLookAtLocked)
	{
		return;
	}

	const auto& mouseState = input.GetMouseDelta();

	const float deltaX = mouseState.x * m_sensitivityX;
	const float deltaY = mouseState.y * m_sensitivityY;

	if (fabsf(deltaX) > kMinMouseDelta)
	{
		auto* myGameObject = GetGameObject();
		if (!myGameObject)
			return;

		auto* fsm = myGameObject->GetComponent<FSMComponent>();
		if (!fsm)
			return;

		auto&				transform = myGameObject->GetTransform();
		auto				pos = transform.GetPosition();
		auto				rot = transform.GetRotation();
		FB_STRUCTS::PosInfo posInfo{{pos.x, pos.y, pos.z}, {rot.x, rot.y, rot.z}};

		const auto curState{fsm->GetCurStateType()};

		if (curState == FB_ENUMS::PLAYER_STATE_TYPE_DEAD)
			return;

		RotateYaw(deltaX);

		{
			// 회전 후 transform에서 값을 읽어 패킷 전송
			auto&				transform = myGameObject->GetTransform();
			auto				pos = transform.GetPosition();
			auto				rot = transform.GetRotation(); // 이제 회전된 값
			FB_STRUCTS::PosInfo posInfo{{pos.x, pos.y, pos.z}, {rot.x, rot.y, rot.z}};
			auto				pb = NetBridge::C2S::Make_CS_MOVE_PACKET(&posInfo);
			GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
		}
	}

	if (m_cameraObjectHandle.IsValid() && fabsf(deltaY) > kMinMouseDelta)
	{
		auto* myGameObject = GetGameObject();
		if (!myGameObject)
			return;

		auto* fsm = myGameObject->GetComponent<FSMComponent>();
		if (!fsm)
			return;

		const auto curState{fsm->GetCurStateType()};

		if (curState == FB_ENUMS::PLAYER_STATE_TYPE_DEAD)
			return;

		RotatePitch(deltaY);
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
	bool  w = input.GetInput('W');
	bool  s = input.GetInput('S');
	bool  a = input.GetInput('A');
	bool  d = input.GetInput('D');

	// 락온 상태 & 카메라 방향 업데이트
	bool isLockOn = false;
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

	bool isMovingInput = (w || s || a || d);

	// 이동 방향 및 캐릭터 회전 설정
	if (isLockOn)
	{
		if (w) fsm->SetMoveDirection(FSMComponent::MoveDirection::FWD);
		else if (s) fsm->SetMoveDirection(FSMComponent::MoveDirection::BWD);
		else if (a) fsm->SetMoveDirection(FSMComponent::MoveDirection::LFT);
		else if (d) fsm->SetMoveDirection(FSMComponent::MoveDirection::RGT);

		movement->SetInputForward(w);
		movement->SetInputBackward(s);
		movement->SetInputLeft(a);
		movement->SetInputRight(d);
	}
	else
	{
		if (w)
			fsm->SetMoveDirection(FSMComponent::MoveDirection::FWD);
		else if (s)
			fsm->SetMoveDirection(FSMComponent::MoveDirection::BWD);
		else if (a)
			fsm->SetMoveDirection(FSMComponent::MoveDirection::LFT);
		else if (d)
			fsm->SetMoveDirection(FSMComponent::MoveDirection::RGT);

		movement->SetInputForward(w);
		movement->SetInputBackward(s);
		movement->SetInputLeft(a);
		movement->SetInputRight(d);

		//fsm->SetMoveDirection(FSMComponent::MoveDirection::FWD);

		//if (isMovingInput)
		//{
		//	// 카메라 기준 방향 계산
		//	XMVECTOR baseFwd = XMVector3Normalize(XMVectorSetY(camFwd, 0.0f));
		//	if (XMVector3LengthSq(baseFwd).m128_f32[0] < 1e-6f)
		//		baseFwd = XMVectorSet(0, 0, 1, 0);

		//	XMVECTOR baseRight = XMVector3Normalize(XMVector3Cross(XMVectorSet(0, 1, 0, 0), baseFwd));

		//	XMVECTOR moveDir = XMVectorZero();
		//	if (w) moveDir = XMVectorAdd(moveDir, baseFwd);
		//	if (s) moveDir = XMVectorSubtract(moveDir, baseFwd);
		//	if (a) moveDir = XMVectorSubtract(moveDir, baseRight);
		//	if (d) moveDir = XMVectorAdd(moveDir, baseRight);

		//	moveDir = XMVector3Normalize(moveDir);

		//	// 캐릭터 몸 회전 (Slerp)
		//	float targetYaw = atan2f(XMVectorGetX(moveDir), XMVectorGetZ(moveDir));
		//	XMVECTOR targetRotQ = XMQuaternionRotationAxis(XMVectorSet(0, 1, 0, 0), targetYaw);
		//	
		//	auto& tr = myGameObject->GetTransform();
		//	auto q = tr.GetRotationQuaternion();
		//	XMVECTOR curRotQ = XMLoadFloat4(&q);
		//	XMVECTOR nextRotQ = XMQuaternionSlerp(curRotQ, targetRotQ, 0.2f);
		//	
		//	XMFLOAT4 nextRotF;
		//	XMStoreFloat4(&nextRotF, nextRotQ);
		//	tr.SetRotationQuaternion(nextRotF);

		//	movement->SetInputForward(true);
		//}
		//else
		//{
		//	movement->SetInputForward(false);
		//}
		//movement->SetInputBackward(false);
		//movement->SetInputLeft(false);
		//movement->SetInputRight(false);
	}

	bool hasJustReleased =
		input.GetInputUp('W') || input.GetInputUp('A') || input.GetInputUp('S') || input.GetInputUp('D');

	bool hasJustPressed =
		input.GetInputDown('W') || input.GetInputDown('A') || input.GetInputDown('S') || input.GetInputDown('D');

	bool isRestricted =
		(curState == FB_ENUMS::PLAYER_STATE_TYPE_PRE_DELAY || curState == FB_ENUMS::PLAYER_STATE_TYPE_ATTACK ||
		 curState == FB_ENUMS::PLAYER_STATE_TYPE_POST_DELAY);

	if (isRestricted)
		return;

	auto&				transform = myGameObject->GetTransform();
	auto				pos = transform.GetPosition();
	auto				rot = transform.GetRotation();
	FB_STRUCTS::PosInfo posInfo{{pos.x, pos.y, pos.z}, {rot.x, rot.y, rot.z}};

	if (isRestricted)
	{
		if (isMovingInput)
		{
			auto pbState = NetBridge::C2S::Make_CS_UPDATE_PLAYER_STATE_PACKET(FB_ENUMS::PLAYER_STATE_TYPE_MOVE);
			GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pbState));
		}
		auto pbMove = NetBridge::C2S::Make_CS_MOVE_PACKET(&posInfo);
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pbMove));
		return;
	}
	
	if (isMovingInput)
	{
		if (curState != FB_ENUMS::PLAYER_STATE_TYPE_MOVE || hasJustPressed)
		{
			fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_MOVE);

			auto pbState = NetBridge::C2S::Make_CS_UPDATE_PLAYER_STATE_PACKET(FB_ENUMS::PLAYER_STATE_TYPE_MOVE);
			GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pbState));
		}

		auto pbMove = NetBridge::C2S::Make_CS_MOVE_PACKET(&posInfo);
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pbMove));
	}
	else
	{
		if (curState == FB_ENUMS::PLAYER_STATE_TYPE_MOVE || hasJustReleased)
		{
			fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_IDLE);

			auto pbState = NetBridge::C2S::Make_CS_UPDATE_PLAYER_STATE_PACKET(FB_ENUMS::PLAYER_STATE_TYPE_IDLE);
			GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pbState));

			auto pbMove = NetBridge::C2S::Make_CS_MOVE_PACKET(&posInfo);
			GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pbMove));
		}
	}

	if (input.GetInputDown('F'))
	{
		auto pb = NetBridge::C2S::Make_CS_GEN_NPC_GENREAL_PACKET();
		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	}
}

void PlayerControllerComponent::RotateYaw(float deltaDegrees)
{
	auto* myGameObject = GetGameObject();
	if (!myGameObject)
	{
		return;
	}

	auto& bodyTransform = myGameObject->GetTransform();

	XMFLOAT4 currentRotF = bodyTransform.GetRotationQuaternion();
	XMVECTOR currentRot = XMLoadFloat4(&currentRotF);
	XMVECTOR yawRotation =
		XMQuaternionRotationAxis(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), XMConvertToRadians(deltaDegrees));

	XMVECTOR newRot = XMQuaternionNormalize(XMQuaternionMultiply(yawRotation, currentRot));

	XMFLOAT4 newRotF;
	XMStoreFloat4(&newRotF, newRot);
	bodyTransform.SetRotationQuaternion(newRotF);
}

void PlayerControllerComponent::RotatePitch(float deltaDegrees)
{
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

	m_pitch += deltaDegrees;
	m_pitch = std::clamp(m_pitch, m_minPitch, m_maxPitch);

	auto* camComp = camObj->GetComponent<CameraComponent>();
	if (camComp)
	{
		camComp->SetLookAtRotationOffset({m_pitch, 0.0f, 0.0f});
	}
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

		// 적 방향 계산 (수평)
		XMVECTOR dirToTarget = XMVectorSubtract(enemyPos, playerPos);
		XMVECTOR dirH = XMVector3Normalize(XMVectorSetY(dirToTarget, 0.0f));

		if (XMVector3LengthSq(dirH).m128_f32[0] < 1e-6f)
		{
			XMFLOAT3 fwd = playerTr->GetForward();
			dirH = XMLoadFloat3(&fwd);
		}

		// 오른쪽 방향
		XMVECTOR rightH = XMVector3Normalize(XMVector3Cross(XMVectorSet(0, 1, 0, 0), dirH));

		// 숄더뷰 오프셋(오른쪽, 위, 뒤)
		XMVECTOR offset = XMVectorScale(rightH, 3.0f) + XMVectorSet(0, 3.5f, 0, 0) + XMVectorScale(dirH, -3.5f);

		XMFLOAT3 offsetF;
		XMStoreFloat3(&offsetF, offset);

		camComp->SetFollowOffset(offsetF); // 월드 오프셋 적용
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