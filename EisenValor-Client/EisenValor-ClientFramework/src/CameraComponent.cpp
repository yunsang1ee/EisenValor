#include "stdafxClientFramework.h"
#include "CameraComponent.h"
#include "SceneGlobal.h"
#include "Scene.h"
#include "Transform.h"

using namespace DirectX;

namespace
{
constexpr float kFovAnimThreshold = 1e-4f;

constexpr float kParallelThreshold = 1e-4f;
} // namespace

CameraComponent::Handle CameraComponent::s_mainCameraHandle = CameraComponent::Handle::Invalid();

void CameraComponent::OnLateUpdate(float deltaTime)
{
	UpdateFovAnimation(deltaTime);

	if (m_lookAt.targetHandle.IsValid())
	{
		UpdateLookAtTarget(deltaTime);
	}

	if (m_viewDirty)
	{
		UpdateViewMatrix();
		m_viewDirty = false;
	}

	if (m_projectionDirty) [[unlikely]]
	{
		UpdateProjectionMatrix();
		m_projectionDirty = false;
	}
}

void CameraComponent::OnDestroy()
{
	if (s_mainCameraHandle == this->GetHandle())
	{
		s_mainCameraHandle = Handle::Invalid();
	}
}

XMMATRIX CameraComponent::GetViewMatrix() const
{
	return XMLoadFloat4x4(&m_cachedViewMatrix);
}

XMMATRIX CameraComponent::GetProjectionMatrix() const
{
	return XMLoadFloat4x4(&m_cachedProjectionMatrix);
}

XMMATRIX CameraComponent::GetMainViewMatrix()
{
	if (auto* mainCamera = GetMainCamera())
	{
		return mainCamera->GetViewMatrix();
	}
	return XMMatrixIdentity();
}

XMMATRIX CameraComponent::GetMainProjectionMatrix()
{
	if (auto* mainCamera = GetMainCamera())
	{
		return mainCamera->GetProjectionMatrix();
	}
	return XMMatrixIdentity();
}

CameraComponent* CameraComponent::GetMainCamera()
{
	if (!s_mainCameraHandle.IsValid())
	{
		return nullptr;
	}

	auto* scene = GLOBAL(SceneGlobal).GetActiveScene();
	if (!scene)
	{
		return nullptr;
	}

	auto* storage = scene->GetStorage<CameraComponent>();
	return storage ? storage->Get(s_mainCameraHandle) : nullptr;
}

void CameraComponent::SetPerspective(float fovY, float aspectRatio, float nearZ, float farZ)
{
	m_projectionType = ProjectionType::Perspective;
	m_perspective.fov = fovY;
	m_perspective.targetFov = fovY;
	m_perspective.aspectRatio = aspectRatio;
	m_nearZ = nearZ;
	m_farZ = farZ;
	m_projectionDirty = true;
}

void CameraComponent::SetOrthographic(float width, float height, float nearZ, float farZ)
{
	m_projectionType = ProjectionType::Orthographic;
	m_orthographic.width = width;
	m_orthographic.height = height;
	m_nearZ = nearZ;
	m_farZ = farZ;
	m_projectionDirty = true;
}

void CameraComponent::SetFov(float fovY)
{
	m_perspective.fov = fovY;
	m_perspective.targetFov = fovY;
	m_projectionDirty = true;
}

void CameraComponent::SetFovAnimated(float targetFovY, float speed)
{
	m_perspective.targetFov = targetFovY;
	m_perspective.fovAnimSpeed = speed;
}

void CameraComponent::UpdateFovAnimation(float deltaTime)
{
	if (m_projectionType != ProjectionType::Perspective)
	{
		return;
	}

	if (fabsf(m_perspective.fov - m_perspective.targetFov) < kFovAnimThreshold)
	{
		return;
	}

	if (m_perspective.fovAnimSpeed <= 0.0f)
	{
		m_perspective.fov = m_perspective.targetFov;
	}
	else
	{
		float t = 1.0f - expf(-m_perspective.fovAnimSpeed * deltaTime);
		m_perspective.fov = std::lerp(m_perspective.fov, m_perspective.targetFov, t);
	}

	m_projectionDirty = true;
}

void CameraComponent::SetLookAtTarget(DenseListHandle<Transform> targetTransform, std::optional<DX::XMFLOAT3> upVector)
{
	m_lookAt.targetHandle = targetTransform;

	if (upVector.has_value())
	{
		m_lookAt.upVector = upVector.value();
	}

	m_viewDirty = true;
	DEBUG_LOG_FMT(
		"[CameraComponent] Set look-at target (Handle: {}) {}\n", targetTransform.GetValue(),
		upVector.has_value() ? "with custom up vector" : "with default up vector"
	);
}

void CameraComponent::SetLookAtTarget(DenseListHandle<GameObject> targetObject, std::optional<DX::XMFLOAT3> upVector)
{
	auto* scene = GLOBAL(SceneGlobal).GetActiveScene();
	if (!scene)
	{
		return;
	}

	auto* targetGameObject = scene->TryGetGameObject(targetObject);
	if (!targetGameObject)
	{
		return;
	}
	auto transformHandle = targetGameObject->GetComponentHandle<Transform>();
	SetLookAtTarget(transformHandle, upVector);
}

void CameraComponent::ClearLookAtTarget()
{
	m_lookAt.targetHandle = HandleOf<Transform>::Invalid();
	m_viewDirty = true;
	DEBUG_LOG_FMT("[CameraComponent] Cleared look-at target\n");
}

void CameraComponent::LookAt(const XMFLOAT3& target, std::optional<DX::XMFLOAT3> upVector)
{
	ClearLookAtTarget();

	if (upVector.has_value())
	{
		m_lookAt.upVector = upVector.value();
	}

	auto* gameObject = GetGameObject();
	if (!gameObject)
	{
		return;
	}

	auto& transform = gameObject->GetTransform();

	XMFLOAT3 cameraPos = transform.GetWorldPosition();
	XMVECTOR eyeVec = XMLoadFloat3(&cameraPos);
	XMVECTOR targetVec = XMLoadFloat3(&target);

	XMVECTOR worldUp = upVector.has_value() ? XMLoadFloat3(&upVector.value()) : XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);


	XMVECTOR forward = XMVector3Normalize(XMVectorSubtract(targetVec, eyeVec));

	float dotFU = fabsf(XMVectorGetX(XMVector3Dot(forward, worldUp)));
	if (dotFU > (1.0f - kParallelThreshold))
	{
		worldUp = XMVectorSet(0.f, 0.f, 1.f, 0.f);
		dotFU = fabsf(XMVectorGetX(XMVector3Dot(forward, worldUp)));
		if (dotFU > (1.0f - kParallelThreshold))
		{
			worldUp = XMVectorSet(1.f, 0.f, 0.f, 0.f);
		}
	}

	XMVECTOR right = XMVector3Normalize(XMVector3Cross(worldUp, forward));
	XMVECTOR up = XMVector3Normalize(XMVector3Cross(forward, right));

	XMMATRIX rotationMatrix = XMMatrixIdentity();
	rotationMatrix.r[0] = right;
	rotationMatrix.r[1] = up;
	rotationMatrix.r[2] = forward;

	XMVECTOR worldQuat = XMQuaternionNormalize(XMQuaternionRotationMatrix(rotationMatrix));

	if (transform.GetParent().IsValid())
	{
		auto* scene = GLOBAL(SceneGlobal).GetActiveScene();
		auto* storage = scene ? scene->GetStorage<Transform>() : nullptr;
		if (auto* parent = storage ? storage->Get(transform.GetParent()) : nullptr)
		{
			XMFLOAT4 parentWorldQf = parent->GetWorldRotationQuaternion();
			XMVECTOR parentWorldQ = XMLoadFloat4(&parentWorldQf);

			worldQuat = XMQuaternionMultiply(XMQuaternionInverse(parentWorldQ), worldQuat);
			worldQuat = XMQuaternionNormalize(worldQuat);
		}
	}

	XMFLOAT4 rotation;
	XMStoreFloat4(&rotation, worldQuat);

	transform.SetRotationQuaternion(rotation);
	m_viewDirty = true;
}

void CameraComponent::SetLookAtUpVector(const DX::XMFLOAT3& upVector)
{
	m_lookAt.upVector = upVector;
	m_viewDirty = true;
	DEBUG_LOG_FMT("[CameraComponent] Set look-at up vector: ({}, {}, {})\n", upVector.x, upVector.y, upVector.z);
}

void CameraComponent::SetFollowOffset(const DX::XMFLOAT3& offset)
{
	m_follow.offset = offset;
	m_follow.useLocalOffset = false;
	m_viewDirty = true;

	DEBUG_LOG_FMT("[CameraComponent] Follow offset set (World): ({}, {}, {})\n", offset.x, offset.y, offset.z);
}

void CameraComponent::SetFollowOffsetLocal(const DX::XMFLOAT3& localOffset)
{
	m_follow.offset = localOffset;
	m_follow.useLocalOffset = true;
	m_viewDirty = true;

	DEBUG_LOG_FMT(
		"[CameraComponent] Follow offset set (Local): ({}, {}, {})\n", localOffset.x, localOffset.y, localOffset.z
	);
}

void CameraComponent::SetSmoothFollow(bool enable, float positionSpeed, float rotationSpeed)
{
	m_follow.smoothEnabled = enable;
	m_follow.positionSpeed = positionSpeed;
	m_follow.rotationSpeed = rotationSpeed;
	DEBUG_LOG_FMT(
		"[CameraComponent] Smooth follow: {} (PosSpeed={}, RotSpeed={})\n", enable ? "enabled" : "disabled",
		positionSpeed, rotationSpeed
	);
}

void CameraComponent::SetAsMainCamera()
{
	s_mainCameraHandle = GetHandle();
	DEBUG_LOG_FMT("[CameraComponent] Set as main camera (Handle: {})\n", s_mainCameraHandle.GetValue());
}

void CameraComponent::UpdateLookAtTarget(float deltaTime)
{
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

	auto* targetTransform = transformStorage->Get(m_lookAt.targetHandle);
	if (!targetTransform)
	{
		m_lookAt.targetHandle = HandleOf<Transform>::Invalid();
		return;
	}

	auto* gameObject = GetGameObject();
	if (!gameObject)
	{
		return;
	}
	auto& cameraTransform = gameObject->GetTransform();

	// Camera Position
	const XMFLOAT3 targetPos = targetTransform->GetWorldPosition();
	XMVECTOR	   targetPosVec = XMLoadFloat3(&targetPos);
	XMVECTOR	   offsetVec = XMLoadFloat3(&m_follow.offset);

	if (m_follow.useLocalOffset)
	{
		XMFLOAT4 targetRotQuat = targetTransform->GetWorldRotationQuaternion();
		XMVECTOR rotQuat = XMLoadFloat4(&targetRotQuat);
		offsetVec = XMVector3Rotate(offsetVec, rotQuat);
	}
	XMVECTOR desiredCameraPosVec = XMVectorAdd(targetPosVec, offsetVec);

	// Smooth Follow Position
	const XMFLOAT3 currentCameraPos = cameraTransform.GetWorldPosition();
	XMVECTOR	   currentCameraPosVec = XMLoadFloat3(&currentCameraPos);

	XMVECTOR finalCameraPosVec = desiredCameraPosVec;

	if (m_follow.smoothEnabled && deltaTime > 0.0f)
	{
		float t = 1.0f - expf(-m_follow.positionSpeed * deltaTime);
		finalCameraPosVec = XMVectorLerp(currentCameraPosVec, desiredCameraPosVec, t);
	}

	XMFLOAT3 finalCameraPos;
	XMStoreFloat3(&finalCameraPos, finalCameraPosVec);
	cameraTransform.SetWorldPosition(finalCameraPos);

	// LookAt Target Rotation
	XMVECTOR eyeVec = finalCameraPosVec;
	XMVECTOR targetVec = targetPosVec;
	XMVECTOR forward = XMVector3Normalize(XMVectorSubtract(targetVec, eyeVec));

	XMFLOAT3 upVector = m_lookAt.upVector;
	if (m_lookAt.followTargetUp)
	{
		upVector = targetTransform->GetUp();
	}
	XMVECTOR upVec = XMLoadFloat3(&upVector);

	float dotFU = fabsf(XMVectorGetX(XMVector3Dot(forward, upVec)));
	if (dotFU > (1.0f - kParallelThreshold))
	{
		// Degenerate Case
		upVec = XMVectorSet(0.f, 0.f, 1.f, 0.f);
		dotFU = fabsf(XMVectorGetX(XMVector3Dot(forward, upVec)));
		if (dotFU > (1.0f - kParallelThreshold))
		{
			upVec = XMVectorSet(1.f, 0.f, 0.f, 0.f);
		}
	}

	XMVECTOR right = XMVector3Normalize(XMVector3Cross(upVec, forward));
	XMVECTOR up = XMVector3Normalize(XMVector3Cross(forward, right));

	XMMATRIX rotationMatrix = XMMatrixIdentity();
	rotationMatrix.r[0] = right;
	rotationMatrix.r[1] = up;
	rotationMatrix.r[2] = forward;

	XMVECTOR desiredWorldQ = XMQuaternionNormalize(XMQuaternionRotationMatrix(rotationMatrix));

	// Smooth Follow Rotation
	XMVECTOR finalWorldQ = desiredWorldQ;
	if (m_follow.smoothEnabled && deltaTime > 0.0f)
	{
		XMFLOAT4 currentWorldQf = cameraTransform.GetWorldRotationQuaternion();
		XMVECTOR currentWorldQ = XMLoadFloat4(&currentWorldQf);
		float	 t = 1.0f - expf(-m_follow.rotationSpeed * deltaTime);
		finalWorldQ = XMQuaternionSlerp(currentWorldQ, desiredWorldQ, t);
		finalWorldQ = XMQuaternionNormalize(finalWorldQ);
	}

	// World To Local Rotation
	XMVECTOR localQ = finalWorldQ;

	if (cameraTransform.GetParent().IsValid())
	{
		if (auto* parent = transformStorage->Get(cameraTransform.GetParent()))
		{
			XMFLOAT4 parentWorldQf = parent->GetWorldRotationQuaternion();
			XMVECTOR parentWorldQ = XMLoadFloat4(&parentWorldQf);

			localQ = XMQuaternionMultiply(XMQuaternionInverse(parentWorldQ), finalWorldQ);
			localQ = XMQuaternionNormalize(localQ);
		}
	}

	DirectX::XMFLOAT4 localQf;
	DirectX::XMStoreFloat4(&localQf, localQ);

	cameraTransform.SetRotationQuaternion(localQf);
	m_viewDirty = true;
}

void CameraComponent::UpdateViewMatrix()
{
	auto* gameObject = GetGameObject();
	if (!gameObject)
	{
		XMStoreFloat4x4(&m_cachedViewMatrix, XMMatrixIdentity());
		return;
	}

	auto& transform = gameObject->GetTransform();

	XMFLOAT3 eyePos = transform.GetWorldPosition();
	XMVECTOR eye = XMLoadFloat3(&eyePos);

	XMFLOAT3 forwardDir = transform.GetForward();
	XMVECTOR forward = XMLoadFloat3(&forwardDir);

	XMVECTOR at = XMVectorAdd(eye, forward);

	XMFLOAT3 upDir = transform.GetUp();
	XMVECTOR up = XMLoadFloat3(&upDir);

	XMMATRIX viewMatrix = XMMatrixLookAtLH(eye, at, up);
	XMStoreFloat4x4(&m_cachedViewMatrix, viewMatrix);
}

void CameraComponent::UpdateProjectionMatrix()
{
	XMMATRIX projectionMatrix;

	switch (m_projectionType)
	{
	case ProjectionType::Perspective:
		projectionMatrix = XMMatrixPerspectiveFovLH(m_perspective.fov, m_perspective.aspectRatio, m_nearZ, m_farZ);
		break;

	case ProjectionType::Orthographic:
		projectionMatrix = XMMatrixOrthographicLH(m_orthographic.width, m_orthographic.height, m_nearZ, m_farZ);
		break;

	default:
		projectionMatrix = XMMatrixIdentity();
		break;
	}
	XMStoreFloat4x4(&m_cachedProjectionMatrix, projectionMatrix);
}
