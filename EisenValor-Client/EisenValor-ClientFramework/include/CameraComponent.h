#pragma once
#include "IComponent.h"

enum class CameraMode
{
	NORMAL, // 일반 모드
	COMBAT	// 전투 모드
};

class CameraComponent : public ComponentBase<CameraComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "CameraComponent"; }

	CameraComponent() = default;
	~CameraComponent() override = default;

private:
	CameraMode m_cameraMode = CameraMode::NORMAL;

	// 카메라 설정값들
	float m_normalModeDistance = 10.0f; // 일반 모드 거리
	float m_normalModeHeight = 2.0f;	// 일반 모드 높이
	float m_combatModeDistance = 5.0f;	// 전투 모드 거리
	float m_combatModeHeight = 1.5f;	// 전투 모드 높이

	// 일반 모드 카메라 회전 각도
	float m_normalModeYaw = 0.0f;	// 좌우 회전
	float m_normalModePitch = 0.3f; // 상하 회전

	// 전투 모드 카메라 각도
	float m_pitch = 0.0f;
	float m_yaw = 0.0f;

	// 카메라 회전 속도
	float m_rotationSpeed = 0.01f; // 마우스 드래그 감도

	bool  m_isMouseDragging = false;
	float m_lastMouseX = 0.0f;
	float m_lastMouseY = 0.0f;

public:
	// 카메라 모드 설정
	void	   SetCameraMode(CameraMode mode) { m_cameraMode = mode; }
	CameraMode GetCameraMode() const { return m_cameraMode; }
	void	   ToggleCameraMode()
	{
		m_cameraMode = (m_cameraMode == CameraMode::NORMAL) ? CameraMode::COMBAT : CameraMode::NORMAL;
	}

	// 일반 모드 카메라 회전 각도 설정
	void  SetNormalModeYaw(float yaw) { m_normalModeYaw = yaw; }
	void  SetNormalModePitch(float pitch) { m_normalModePitch = pitch; }
	void  AddNormalModeYaw(float deltaYaw) { m_normalModeYaw += deltaYaw; }
	void  AddNormalModePitch(float deltaPitch) { m_normalModePitch += deltaPitch; }
	float GetNormalModeYaw() const { return m_normalModeYaw; }
	float GetNormalModePitch() const { return m_normalModePitch; }

	// 카메라 설정값들
	void SetNormalModeDistance(float distance) { m_normalModeDistance = distance; }
	void SetNormalModeHeight(float height) { m_normalModeHeight = height; }
	void SetCombatModeDistance(float distance) { m_combatModeDistance = distance; }
	void SetCombatModeHeight(float height) { m_combatModeHeight = height; }
	void SetRotationSpeed(float speed) { m_rotationSpeed = speed; }

	// 뷰 매트릭스 계산
	DirectX::XMMATRIX GetViewMatrix() const;

	// 입력 처리 (마우스 드래그)
	void ProcessMouseDrag(float deltaX, float deltaY);
	void ProcessMouseInput(float mouseX, float mouseY, bool isLeftButtonPressed);
};