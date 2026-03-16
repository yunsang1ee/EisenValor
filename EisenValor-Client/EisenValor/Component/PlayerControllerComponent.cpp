#include "stdafxClient.h"
#include "PlayerControllerComponent.h"
#include "GameObject.h"
#include "InputGlobal.h"
#include "Transform.h"
#include "SceneGlobal.h"
#include "Scene.h"
#include "Component/FSM/FSMComponent.h"

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

			auto pb = NetBridge::C2S::Make_CS_MOVE_PACKET(&posInfo);
			GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
		}
		RotateYaw(deltaX);
	}

	if (m_cameraObjectHandle.IsValid() && fabsf(deltaY) > kMinMouseDelta)
	{
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

		}
		RotatePitch(deltaY);
	}
}

void PlayerControllerComponent::ProcessMovementInput(float deltaTime)
{
	// if (!m_movementHandle.IsValid())
	//{
	//	return;
	// }

	// auto* myGameObject = GetGameObject();
	// if (!myGameObject)
	//{
	//	return;
	// }

	// auto* movement = myGameObject->GetComponent<MovementComponent>();
	// if (!movement)
	//{
	//	return;
	// }

	// auto& input = GLOBAL(InputGlobal);

	// bool isMoving = input.GetInput('W') || input.GetInput('S') || input.GetInput('A') || input.GetInput('D');

	// movement->SetInputForward(input.GetInput('W'));
	// movement->SetInputBackward(input.GetInput('S'));
	// movement->SetInputLeft(input.GetInput('A'));
	// movement->SetInputRight(input.GetInput('D'));

	// if (auto* fsm = myGameObject->GetComponent<FSMComponent>())
	//{
	//	uint8_t curState = fsm->GetCurStateType();

	//	// 공격, 스턴, 사망 상태인 경우 이동/대기로의 전환 방지
	//	bool isRestrictedState = (curState == FB_ENUMS::PLAYER_STATE_TYPE_PRE_DELAY ||
	//							  curState == FB_ENUMS::PLAYER_STATE_TYPE_ATTACK ||
	//							  curState == FB_ENUMS::PLAYER_STATE_TYPE_POST_DELAY ||
	//							  curState == FB_ENUMS::PLAYER_STATE_TYPE_STUN ||
	//							  curState == FB_ENUMS::PLAYER_STATE_TYPE_DEAD);
	//	auto& transform = myGameObject->GetTransform();
	//	auto  pos = transform.GetPosition();
	//	auto  rot = transform.GetRotation();

	//	FB_STRUCTS::Vec3	posVec{pos.x, pos.y, pos.z};
	//	FB_STRUCTS::Vec3	rotVec{rot.x, rot.y, rot.z};
	//	FB_STRUCTS::PosInfo posInfo{posVec, rotVec};
	//	if (!isRestrictedState)
	//	{
	//		if (isMoving)
	//		{
	//			if (fsm->GetCurStateType() != FB_ENUMS::PLAYER_STATE_TYPE_MOVE)
	//				fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_MOVE);
	//			auto pb = NetBridge::C2S::Make_CS_MOVE_PACKET(&posInfo, FB_ENUMS::PLAYER_STATE_TYPE_MOVE);
	//			GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	//		}
	//		else
	//		{
	//			if(fsm->GetCurStateType() == FB_ENUMS::PLAYER_STATE_TYPE_IDLE) {

	//			}
	//			else
	//			{
	//				fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_IDLE);
	//				auto pb = NetBridge::C2S::Make_CS_MOVE_PACKET(&posInfo, FB_ENUMS::PLAYER_STATE_TYPE_IDLE);
	//				GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	//			}
	//		}
	//	}
	//}

	// if (input.GetInputDown('F')) {
	//	auto pb{NetBridge::C2S::Make_CS_GEN_NPC_GENREAL_PACKET()};
	//	GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	// }
	//
	//
	//

	// if (!m_movementHandle.IsValid())
	//	return;

	// auto* myGameObject = GetGameObject();
	// if (!myGameObject)
	//	return;

	// auto* movement = myGameObject->GetComponent<MovementComponent>();
	// if (!movement)
	//	return;

	// auto& input = GLOBAL(InputGlobal);
	// auto* fsm = myGameObject->GetComponent<FSMComponent>();
	// if (!fsm)
	//	return;

	// if (fsm->GetCurStateType() == FB_ENUMS::PLAYER_STATE_TYPE_STUN || fsm->GetCurStateType() ==
	// FB_ENUMS::PLAYER_STATE_TYPE_DEAD) 	return;


	// bool w = input.GetInput('W');
	// bool s = input.GetInput('S');
	// bool a = input.GetInput('A');
	// bool d = input.GetInput('D');

	// bool isMovingNow = (w || s || a || d);

	// movement->SetInputForward(w);
	// movement->SetInputBackward(s);
	// movement->SetInputLeft(a);
	// movement->SetInputRight(d);

	// uint8_t curState = fsm->GetCurStateType();
	// bool	isRestrictedState =
	//	(curState == FB_ENUMS::PLAYER_STATE_TYPE_PRE_DELAY || curState == FB_ENUMS::PLAYER_STATE_TYPE_ATTACK ||
	//	 curState == FB_ENUMS::PLAYER_STATE_TYPE_POST_DELAY || curState == FB_ENUMS::PLAYER_STATE_TYPE_STUN ||
	//	 curState == FB_ENUMS::PLAYER_STATE_TYPE_DEAD);

	// if (isRestrictedState)
	//	return;

	// auto&				transform = myGameObject->GetTransform();
	// auto				pos = transform.GetPosition();
	// auto				rot = transform.GetRotation();
	// FB_STRUCTS::PosInfo posInfo{{pos.x, pos.y, pos.z}, {rot.x, rot.y, rot.z}};

	// if (isMovingNow)
	//{
	//	bool hasJustPressed = input.GetInputDown('W') || input.GetInputDown('A') || input.GetInputDown('S') ||
	//input.GetInputDown('D');

	//	if (curState != FB_ENUMS::PLAYER_STATE_TYPE_MOVE || hasJustPressed)
	//	{
	//		fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_MOVE);
	//		auto pb = NetBridge::C2S::Make_CS_UPDATE_PLAYER_STATE_PACKET(FB_ENUMS::PLAYER_STATE_TYPE_MOVE);
	//		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	//	}

	//	auto pb = NetBridge::C2S::Make_CS_MOVE_PACKET(&posInfo, FB_ENUMS::PLAYER_STATE_TYPE_MOVE);
	//	GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	//}
	// else
	//{
	//	if (curState == FB_ENUMS::PLAYER_STATE_TYPE_MOVE)
	//	{
	//		fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_IDLE);
	//		{
	//			auto pb = NetBridge::C2S::Make_CS_UPDATE_PLAYER_STATE_PACKET(FB_ENUMS::PLAYER_STATE_TYPE_IDLE);
	//			GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	//		}
	//		auto pb = NetBridge::C2S::Make_CS_MOVE_PACKET(&posInfo, FB_ENUMS::PLAYER_STATE_TYPE_IDLE);
	//		GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	//	}
	//}

	// if (input.GetInputDown('F'))
	//{
	//	auto pb{NetBridge::C2S::Make_CS_GEN_NPC_GENREAL_PACKET()};
	//	GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
	// }


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
	if (curState == FB_ENUMS::PLAYER_STATE_TYPE_STUN || curState == FB_ENUMS::PLAYER_STATE_TYPE_DEAD)
		return;

	auto& input = GLOBAL(InputGlobal);
	bool  w = input.GetInput('W');
	bool  s = input.GetInput('S');
	bool  a = input.GetInput('A');
	bool  d = input.GetInput('D');

	bool isMovingInput = (w || s || a || d);

	bool hasJustReleased =
		input.GetInputUp('W') || input.GetInputUp('A') || input.GetInputUp('S') || input.GetInputUp('D');

	bool hasJustPressed =
		input.GetInputDown('W') || input.GetInputDown('A') || input.GetInputDown('S') || input.GetInputDown('D');

	movement->SetInputForward(w);
	movement->SetInputBackward(s);
	movement->SetInputLeft(a);
	movement->SetInputRight(d);

	bool isRestricted =
		(curState == FB_ENUMS::PLAYER_STATE_TYPE_PRE_DELAY || curState == FB_ENUMS::PLAYER_STATE_TYPE_ATTACK ||
		 curState == FB_ENUMS::PLAYER_STATE_TYPE_POST_DELAY);

	if (isRestricted)
		return;

	auto&				transform = myGameObject->GetTransform();
	auto				pos = transform.GetPosition();
	auto				rot = transform.GetRotation();
	FB_STRUCTS::PosInfo posInfo{{pos.x, pos.y, pos.z}, {rot.x, rot.y, rot.z}};

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

	// 6. 기타 상호작용
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
		XMVECTOR offset = XMVectorScale(rightH, 2.0f) + XMVectorSet(0, 1.2f, 0, 0) + XMVectorScale(dirH, -3.0f);

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