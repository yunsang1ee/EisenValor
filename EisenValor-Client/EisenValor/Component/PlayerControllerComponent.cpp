#include "stdafxClient.h"
#include "PlayerControllerComponent.h"
#include "GameObject.h"
#include "InputGlobal.h"
#include "Transform.h"
#include "SceneGlobal.h"
#include "Scene.h"

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

	FB_STRUCTS::Vec3 posVec{pos.x, pos.y, pos.z};
	FB_STRUCTS::Vec3 rotVec{rot.x, rot.y, rot.z};
	FB_STRUCTS::PosInfo posInfo{posVec, rotVec};

	auto pb = NetBridge::C2S::Make_CS_MOVE_PACKET(&posInfo, FB_ENUMS::GENERAL_STATE_TYPE_NONE);
	GLOBAL(NetBridge::NetworkGlobal).Send(std::move(pb));
}

void PlayerControllerComponent::ProcessMouseRotation(float deltaTime)
{
	auto& input = GLOBAL(InputGlobal);

	if (!input.IsMouseLocked() || !input.IsWindowFocused())
	{
		return;
	}

	const auto& mouseState = input.GetMouseDelta();

	const float deltaX = mouseState.x * m_sensitivityX;
	const float deltaY = mouseState.y * m_sensitivityY;

	if (fabsf(deltaX) > kMinMouseDelta)
	{
		RotateYaw(deltaX);
	}

	if (m_cameraObjectHandle.IsValid() && fabsf(deltaY) > kMinMouseDelta)
	{
		RotatePitch(deltaY);
	}
}

void PlayerControllerComponent::ProcessMovementInput(float deltaTime)
{
	if (!m_movementHandle.IsValid())
	{
		return;
	}

	auto* myGameObject = GetGameObject();
	if (!myGameObject)
	{
		return;
	}

	auto* movement = myGameObject->GetComponent<MovementComponent>();
	if (!movement)
	{
		return;
	}

	auto& input = GLOBAL(InputGlobal);

	movement->SetInputForward(input.GetInput('W'));
	movement->SetInputBackward(input.GetInput('S'));
	movement->SetInputLeft(input.GetInput('A'));
	movement->SetInputRight(input.GetInput('D'));
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