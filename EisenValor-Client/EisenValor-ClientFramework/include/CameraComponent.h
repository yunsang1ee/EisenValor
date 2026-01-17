#pragma once
#include "IComponent.h"

enum class ProjectionType
{
	Perspective,
	Orthographic
};

// TODO: FOV Animation

class Transform;

class CameraComponent : public ComponentBase<CameraComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "CameraComponent"; }
	static constexpr int		 kPriority = 100;

	using Handle = DenseListHandle<CameraComponent>;

	CameraComponent() = default;
	~CameraComponent() override = default;

	void OnLateUpdate(float deltaTime);
	void OnDestroy() override;

	DX::XMMATRIX GetViewMatrix() const;
	DX::XMMATRIX GetProjectionMatrix() const;

	static DX::XMMATRIX		GetMainViewMatrix();
	static DX::XMMATRIX		GetMainProjectionMatrix();
	static CameraComponent* GetMainCamera();

	void SetPerspective(float fovY, float aspectRatio, float nearZ, float farZ);
	void SetOrthographic(float width, float height, float nearZ, float farZ);

	// ==================================================================================
	// LookAt Target
	// ==================================================================================
	void SetLookAtTarget(
		DenseListHandle<Transform> targetTransform, std::optional<DX::XMFLOAT3> upVector = std::nullopt
	);
	void SetLookAtTarget(DenseListHandle<GameObject> targetObject, std::optional<DX::XMFLOAT3> upVector = std::nullopt);
	void ClearLookAtTarget();

	void LookAt(const DX::XMFLOAT3& target, std::optional<DX::XMFLOAT3> upVector = std::nullopt);

	void				SetLookAtRotationOffset(const DX::XMFLOAT3& offsetEulerAngles);
	const DX::XMFLOAT3& GetLookAtRotationOffset() const { return m_lookAt.rotationOffset; }

	void SetEnableLookAtRotation(bool enable) { m_lookAt.enableLookAtRotation = enable; }

	void SetLookAtUpVector(const DX::XMFLOAT3& upVector);
	void SetFollowTargetUp(bool follow) { m_lookAt.followTargetUp = follow; }

	const DX::XMFLOAT3& GetLookAtUpVector() const { return m_lookAt.upVector; }

	// ==================================================================================
	// Third-Person Camera (Shoulder View)
	// ==================================================================================
	void SetFollowOffset(const DX::XMFLOAT3& offset);
	void SetFollowOffsetLocal(const DX::XMFLOAT3& localOffset);
	void SetSmoothFollow(bool enable, float positionSpeed = 5.0f, float rotationSpeed = 5.0f);

	const DX::XMFLOAT3& GetFollowOffset() const { return m_follow.offset; }
	bool				IsSmoothFollowEnabled() const { return m_follow.smoothEnabled; }

	// ==================================================================================
	// FOV Animation
	// ==================================================================================
	void SetFov(float fovY);
	void SetFovAnimated(float targetFovY, float speed = 5.0f);

	void SetAsMainCamera();

	ProjectionType GetProjectionType() const { return m_projectionType; }
	float		   GetFOV() const { return m_perspective.fov; }
	float		   GetAspectRatio() const { return m_perspective.aspectRatio; }
	float		   GetNearZ() const { return m_nearZ; }
	float		   GetFarZ() const { return m_farZ; }

	bool HasLookAtTarget() const { return m_lookAt.targetHandle.IsValid(); }

private:
	void UpdateLookAtTarget(float deltaTime);
	void UpdateFovAnimation(float deltaTime);
	void UpdateViewMatrix();
	void UpdateProjectionMatrix();

private:
	struct PerspectiveParams
	{
		float fov = DX::XM_PIDIV4;
		float aspectRatio = 16.0f / 9.0f;
		float targetFov = DX::XM_PIDIV4;
		float fovAnimSpeed = 0.0f;
	};

	struct OrthographicParams
	{
		float width = 10.0f;
		float height = 10.0f;
	};

	struct LookAtSettings
	{
		DenseListHandle<Transform> targetHandle = {};
		DirectX::XMFLOAT3		   upVector = {0.0f, 1.0f, 0.0f};
		DirectX::XMFLOAT3		   rotationOffset = {0.0f, 0.0f, 0.0f};
		bool					   followTargetUp = false;
		bool					   enableLookAtRotation = true; 
	};

	struct FollowSettings
	{
		DirectX::XMFLOAT3 offset = {0.0f, 0.0f, 0.0f};
		float			  positionSpeed = 5.0f;
		float			  rotationSpeed = 5.0f;
		bool			  smoothEnabled = false;
		bool			  useLocalOffset = false;
	};

private:
	static Handle s_mainCameraHandle;

	DirectX::XMFLOAT4X4 m_cachedViewMatrix;
	DirectX::XMFLOAT4X4 m_cachedProjectionMatrix;

	// Projection
	ProjectionType	   m_projectionType = ProjectionType::Perspective;
	float			   m_nearZ = 0.1f;
	float			   m_farZ = 1000.0f;
	PerspectiveParams  m_perspective;
	OrthographicParams m_orthographic;

	// Camera Control
	LookAtSettings m_lookAt;
	FollowSettings m_follow;

	// Flags
	bool m_viewDirty = true;
	bool m_projectionDirty = true;
};