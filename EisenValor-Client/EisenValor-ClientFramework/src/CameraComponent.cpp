#include "stdafxClientFramework.h"
#include "CameraComponent.h"
#include "GameObject.h"
#include "Scene.h"
#include "SceneGlobal.h"
#include "Transform.h"

DirectX::XMMATRIX CameraComponent::GetViewMatrix() const
{
	auto* myGameObject = GetGameObject();
	if (!myGameObject)
	{
		return DirectX::XMMatrixIdentity();
	}

	// Transform 컴포넌트 가져오기
	auto&		 transform = myGameObject->GetTransform();
	DX::XMFLOAT3 playerPos = transform.GetPosition();
	DX::XMFLOAT3 playerRot = transform.GetRotation();

	if (m_cameraMode == CameraMode::NORMAL)
	{
		float totalYaw = playerRot.y + m_normalModeYaw;

		// 카메라 위치 계산 (플레이어 뒤)
		float cameraX = playerPos.x - sinf(totalYaw) * m_normalModeDistance;
		float cameraZ = playerPos.z - cosf(totalYaw) * m_normalModeDistance;
		float cameraY = playerPos.y + m_normalModeHeight + sinf(m_normalModePitch) * m_normalModeDistance;

		DirectX::XMFLOAT3 cameraPos(cameraX, cameraY, cameraZ);
		DirectX::XMFLOAT3 target(playerPos.x, playerPos.y, playerPos.z);
		DirectX::XMFLOAT3 up(0.0f, 1.0f, 0.0f);

		return DirectX::XMMatrixLookAtLH(
			DirectX::XMLoadFloat3(&cameraPos), DirectX::XMLoadFloat3(&target), DirectX::XMLoadFloat3(&up)
		);
	}
	else
	{
		// 전투 모드: 플레이어 뒤에 고정된 카메라
		// 플레이어 뒤에 카메라 배치
		float cameraX = playerPos.x - sinf(playerRot.y) * m_combatModeDistance;
		float cameraZ = playerPos.z - cosf(playerRot.y) * m_combatModeDistance;
		float cameraY = playerPos.y + m_combatModeHeight;

		DirectX::XMFLOAT3 cameraPos(cameraX, cameraY, cameraZ);
		DirectX::XMFLOAT3 target(playerPos.x, playerPos.y + 0.8f, playerPos.z);
		DirectX::XMFLOAT3 up(0.0f, 1.0f, 0.0f);

		return DirectX::XMMatrixLookAtLH(
			DirectX::XMLoadFloat3(&cameraPos), DirectX::XMLoadFloat3(&target), DirectX::XMLoadFloat3(&up)
		);
	}
}

void CameraComponent::ProcessMouseDrag(float deltaX, float deltaY)
{
	if (m_cameraMode == CameraMode::NORMAL)
	{
		// 일반 모드에서만 마우스 드래그로 카메라 회전
		m_normalModeYaw += deltaX * m_rotationSpeed;
		m_normalModePitch += deltaY * m_rotationSpeed;

		// 피치 각도 제한 (위아래 회전 제한)
		const float maxPitch = 1.5f;
		const float minPitch = -0.5f;
		if (m_normalModePitch > maxPitch)
			m_normalModePitch = maxPitch;
		if (m_normalModePitch < minPitch)
			m_normalModePitch = minPitch;
	}
}

void CameraComponent::ProcessMouseInput(float mouseX, float mouseY, bool isLeftButtonPressed)
{
	if (m_cameraMode == CameraMode::NORMAL)
	{
		if (isLeftButtonPressed)
		{
			if (!m_isMouseDragging)
			{
				m_isMouseDragging = true;
				m_lastMouseX = mouseX;
				m_lastMouseY = mouseY;
			}
			else
			{
				// 움직임 감지
				float deltaX = mouseX - m_lastMouseX;
				float deltaY = mouseY - m_lastMouseY;

				if (abs(deltaX) > 0.1f || abs(deltaY) > 0.1f)
				{
					ProcessMouseDrag(deltaX, deltaY);
				}

				m_lastMouseX = mouseX;
				m_lastMouseY = mouseY;
			}
		}
		else
		{
			if (m_isMouseDragging)
			{
				m_isMouseDragging = false;
			}
		}
	}
}