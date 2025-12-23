#include "stdafxClientFramework.h"
#include "CameraComponent.h"

// Static 멤버 초기화
DirectX::XMMATRIX CameraComponent::s_mainViewMatrix = DirectX::XMMatrixIdentity();
DirectX::XMMATRIX CameraComponent::s_mainProjectionMatrix = DirectX::XMMatrixIdentity();

CameraComponent::CameraComponent()
	: m_viewMatrix(DirectX::XMMatrixIdentity()), m_projectionMatrix(DirectX::XMMatrixIdentity())
{
}