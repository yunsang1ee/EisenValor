#pragma once
#include "IRenderData.h"
#include <DirectXMath.h>

class CameraRenderData : public RenderDataBase<CameraRenderData>
{
public:
	CameraRenderData() = default;
	virtual ~CameraRenderData() override = default;

	DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX viewProjInverse = DirectX::XMMatrixIdentity();

	DirectX::XMFLOAT3 cameraPosition = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 cameraDirection = { 0.0f, 0.0f, 1.0f };
	float			  nearZ = 0.1f;
	float			  farZ = 1000.0f;
	float			  fov = 60.0f;
	float			  aspectRatio = 1.0f;
};
