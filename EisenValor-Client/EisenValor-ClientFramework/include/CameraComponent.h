#pragma once
#include "IComponent.h"

enum class ProjectionType
{
	Perspective,
	Orthographic
};

class CameraComponent : public ComponentBase<CameraComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "CameraComponent"; }

	CameraComponent();
	~CameraComponent() override = default;

	// Static 멤버
	static DirectX::XMMATRIX GetMainViewMatrix() { return s_mainViewMatrix; }
	static DirectX::XMMATRIX GetMainProjectionMatrix() { return s_mainProjectionMatrix; }
	static void				 SetMainViewMatrix(const DirectX::XMMATRIX& matrix) { s_mainViewMatrix = matrix; }
	static void SetMainProjectionMatrix(const DirectX::XMMATRIX& matrix) { s_mainProjectionMatrix = matrix; }

	// View Matrix (매 프레임 업데이트)
	DirectX::XMMATRIX GetViewMatrix() const { return m_viewMatrix; }
	void SetViewMatrix(const DirectX::XMMATRIX& matrix) { m_viewMatrix = matrix; }

	// View Matrix 생성용 데이터
	DirectX::XMFLOAT3 GetEyePosition() const { return m_eye; }
	DirectX::XMFLOAT3 GetAtPosition() const { return m_at; }
	DirectX::XMFLOAT3 GetUpVector() const { return m_up; }

	void SetEyePosition(const DirectX::XMFLOAT3& eye) { m_eye = eye; }
	void SetAtPosition(const DirectX::XMFLOAT3& at) { m_at = at; }
	void SetUpVector(const DirectX::XMFLOAT3& up) { m_up = up; }

	// View Matrix 업데이트
	void UpdateViewMatrix()
	{
		DirectX::XMVECTOR eyeVec = DirectX::XMLoadFloat3(&m_eye);
		DirectX::XMVECTOR atVec = DirectX::XMLoadFloat3(&m_at);
		DirectX::XMVECTOR upVec = DirectX::XMLoadFloat3(&m_up);
		m_viewMatrix = DirectX::XMMatrixLookAtLH(eyeVec, atVec, upVec);
	}

	// Projection Matrix (초기화 시에만)
	DirectX::XMMATRIX GetProjectionMatrix() const { return m_projectionMatrix; }
	void			  SetProjectionMatrix(const DirectX::XMMATRIX& matrix) { m_projectionMatrix = matrix; }

	// Projection Matrix 생성용 데이터
	ProjectionType GetProjectionType() const { return m_projectionType; }
	float		   GetFOV() const { return m_fov; }
	float		   GetNearZ() const { return m_nearZ; }
	float		   GetFarZ() const { return m_farZ; }
	float		   GetSize() const { return m_size; }

	void SetProjectionType(ProjectionType type) { m_projectionType = type; }
	void SetFOV(float fov) { m_fov = fov; }
	void SetNearFar(float nearZ, float farZ)
	{
		m_nearZ = nearZ;
		m_farZ = farZ;
	}
	void SetSize(float size) { m_size = size; }

	// Projection Matrix 생성 (초기화 시에만 호출)
	void CreateProjectionMatrix(float aspectRatio)
	{
		switch (m_projectionType)
		{
		case ProjectionType::Perspective:
			m_projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(m_fov, aspectRatio, m_nearZ, m_farZ);
			break;
		case ProjectionType::Orthographic:
		{
			float width = m_size * aspectRatio;
			float height = m_size;
			m_projectionMatrix = DirectX::XMMatrixOrthographicLH(width, height, m_nearZ, m_farZ);
		}
		break;
		}
	}

	// 메인 카메라 설정
	void SetAsMainCamera()
	{
		s_mainViewMatrix = m_viewMatrix;
		s_mainProjectionMatrix = m_projectionMatrix;
	}

private:
	// Static 멤버
	static DirectX::XMMATRIX s_mainViewMatrix;
	static DirectX::XMMATRIX s_mainProjectionMatrix;

	// View Matrix
	DirectX::XMMATRIX m_viewMatrix;
	DirectX::XMFLOAT3 m_eye = {0.0f, 0.0f, -10.0f};
	DirectX::XMFLOAT3 m_at = {0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT3 m_up = {0.0f, 1.0f, 0.0f};

	// Projection Matrix
	DirectX::XMMATRIX m_projectionMatrix;
	ProjectionType	  m_projectionType = ProjectionType::Perspective;
	float			  m_fov = DirectX::XM_PIDIV4;
	float			  m_nearZ = 0.1f;
	float			  m_farZ = 1000.0f;
	float			  m_size = 1.0f;
};