#pragma once
#include "RenderDataPolicy.h"
#include <DirectXMath.h>

class CameraRenderData : public RenderDataBase<CameraRenderData>
{
public:
	CameraRenderData() = default;
	virtual ~CameraRenderData() override = default;

	DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX viewProjInverse = DirectX::XMMatrixIdentity();
	// Top-left-origin subpixel offset, in render pixels. This is the shared camera-jitter
	// contract for ray generation, motion vectors, and future DLSS/Streamline consumers.
	DirectX::XMFLOAT2 jitterPixels = {0.0f, 0.0f};
	DirectX::XMFLOAT2 previousJitterPixels = {0.0f, 0.0f};
	uint32_t		  jitterSequenceIndex = 0;

	DirectX::XMFLOAT3 cameraPosition = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 previousCameraPosition = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 cameraDirection = { 0.0f, 0.0f, 1.0f };
	float			  nearZ = 0.1f;
	float			  farZ = 1000.0f;
	float			  fov = 60.0f;
	float			  aspectRatio = 1.0f;
};
