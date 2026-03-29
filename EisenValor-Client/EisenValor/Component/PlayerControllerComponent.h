#pragma once
#include "IComponent.h"
#include "MovementComponent.h"

class PlayerControllerComponent : public ComponentBase<PlayerControllerComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "PlayerControllerComponent"; }

	void OnStart() override;
	void OnDestroy() override;
	void OnFixedUpdate(float deltaTime);
	void OnUpdate(float deltaTime);

	void  SetMouseSensitivity(float x, float y);
	void  SetPitchLimits(float minPitch, float maxPitch);
	float GetPitch() const { return m_pitch; }

	void SetCameraHandle(HandleOf<GameObject> cameraHandle)
	{
		m_cameraObjectHandle = cameraHandle;
		InitializePitchFromCamera();
	}

private:
	void ProcessMouseRotation(float deltaTime);
	void ProcessMovementInput(float deltaTime);

	void RotateYaw(float deltaDegrees);
	void RotatePitch(float deltaDegrees);

	void UpdateCameraShoulderView(class CameraComponent* camComp);
	void RotatePlayerToTarget(class CameraComponent* camComp);

	void FindCameraInChildren(GameObject* parentGameObject);
	void InitializePitchFromCamera();

private:
	HandleOf<GameObject>		m_cameraObjectHandle;
	HandleOf<MovementComponent> m_movementHandle;

	float m_pitch = 0.0f;

	float m_sensitivityX = 0.1f;
	float m_sensitivityY = 0.1f;

	float m_minPitch = -89.0f;
	float m_maxPitch = 89.0f;

	// Shoulder view camera vertical offset (raise/lower camera height)
	static constexpr float kShoulderViewVertical = 1.5f;

};