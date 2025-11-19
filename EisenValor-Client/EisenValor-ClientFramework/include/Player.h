#pragma once
#include "stdafxClientFramework.h"
#include "GameObject.h"
#include "DxCommon.h"

class Player : public GameObject
{
public:
	Player() = default;
	virtual ~Player() = default;

	// GameObject 순수 가상 함수 구현
	virtual void Initialize(ID3D12Device* device) override;
	virtual void Update(float deltaTime) override;
	virtual void Render(ID3D12GraphicsCommandList* cmdList, DirectX::XMMATRIX view, DirectX::XMMATRIX projection)
		override;

	// GameObject의 ObjectType 반환
	virtual FB_ENUMS::GAME_OBJECT_TYPE GetObjectType() const override { return FB_ENUMS::GAME_OBJECT_TYPE_PLAYER; }

	// Player 전용 함수들
	void  SetSpeed(float speed) { m_playerSpeed = speed; }
	float GetSpeed() const { return m_playerSpeed; }

	// 카메라 관련 함수들 추가
	float GetCameraYaw() const { return m_cameraYaw; }
	float GetCameraPitch() const { return m_cameraPitch; }
	float GetCameraDistance() const { return m_cameraDistance; }

	void SetCameraYaw(float yaw) { m_cameraYaw = yaw; }
	void SetCameraPitch(float pitch) { m_cameraPitch = pitch; }
	void SetCameraDistance(float distance) { m_cameraDistance = distance; }

	// 카메라 관련 함수 추가
	virtual DirectX::XMMATRIX GetViewMatrix() const;

private:
	Vec3 PredictPosition(const Vec3& pos, const Vec3& vel, const Vec3& acc, double t)
	{
		float tf = static_cast<float>(t);
		float half_t_squared = 0.5f * tf * tf;

		return Vec3{
			pos.x + vel.x * tf + acc.x * half_t_squared, pos.y + vel.y * tf + acc.y * half_t_squared,
			pos.z + vel.z * tf + acc.z * half_t_squared
		};
	}

	Vec3 Lerp(const Vec3& a, const Vec3& b, float t)
	{
		return Vec3{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t};
	}


public:
	virtual void SetTeamColor() override;

protected:
	// 플레이어 속성
	float m_playerSpeed = 5.0f; // 이동 속도

	// 카메라 관련 변수들 추가
	bool  m_isMouseDragging = false;
	float m_cameraYaw = 0.0f;	// 좌우
	float m_cameraPitch = 0.0f; // 위아래
	float m_cameraDistance = 15.0f;
	float m_mouseSensitivity = 0.005f;
	float m_lastMouseX = 0.0f;
	float m_lastMouseY = 0.0f;

	// 렌더링 리소스들
	ComPtr<ID3D12Resource>	 m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

	ComPtr<ID3D12Resource>	m_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

	// 상수 버퍼 추가 25.07.20
	ComPtr<ID3D12Resource> m_constantBuffer;
	ConstantBuffer		   m_constantBufferData;
	UINT8*				   m_pCbvDataBegin = nullptr; // 시작 주소

	// 표시등
	ComPtr<ID3D12Resource> m_constantBuffer3; 
	ConstantBuffer		   m_constantBufferData3;
	UINT8*				   m_pCbvDataBegin3 = nullptr;

	// HP바 렌더링 리소스
	//ComPtr<ID3D12Resource> m_hpBarBackgroundBuffer; // 배경
	//ConstantBuffer		   m_hpBarBackgroundData;
	//UINT8*				   m_pHpBarBackgroundDataBegin = nullptr;

	ComPtr<ID3D12Resource> m_hpBarForegroundBuffer; // HP바 상수 버퍼
	ConstantBuffer		   m_hpBarForegroundData;
	UINT8*				   m_pHpBarForegroundDataBegin = nullptr;
	ComPtr<ID3D12Resource> m_hpBarVertexBuffer;        // HP바 전용 버텍스 버퍼
	D3D12_VERTEX_BUFFER_VIEW m_hpBarVertexBufferView;  // HP바 전용 버텍스 버퍼 뷰

	// 오각형 와이어프레임 관련 리소스 추가
	// 3개의 방패용 상수 버퍼들
	ComPtr<ID3D12Resource>	 m_pentagonConstantBuffer[3];
	ConstantBuffer			 m_pentagonConstantBufferData[3];
	UINT8*					 m_pPentagonDataBegin[3];
	ComPtr<ID3D12Resource>	 m_pentagonVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_pentagonVertexBufferView;
	ComPtr<ID3D12Resource>	 m_pentagonIndexBuffer;
	D3D12_INDEX_BUFFER_VIEW	 m_pentagonIndexBufferView;
	int m_pentagonIndexCount = 0; // 인덱스 개수를 저장할 멤버 변수
	
};