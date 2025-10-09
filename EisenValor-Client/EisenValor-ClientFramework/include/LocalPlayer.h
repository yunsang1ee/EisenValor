#pragma once

#include "Player.h"

class LocalPlayer : public Player
{
private:
	bool										   sendFlag{false};
	std::chrono::high_resolution_clock::time_point lastSend = std::chrono::high_resolution_clock::now();

	// 와이어프레임 박스 관련 멤버 변수들
	ComPtr<ID3D12Resource>	 m_wireFrameVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_wireFrameVertexBufferView;
	ComPtr<ID3D12Resource>	 m_wireFrameIndexBuffer;
	D3D12_INDEX_BUFFER_VIEW	 m_wireFrameIndexBufferView;
	ComPtr<ID3D12Resource>	 m_wireFrameConstantBuffer;
	ConstantBuffer			 m_wireFrameConstantBufferData;
	UINT8*					 m_pWireFrameCbvDataBegin = nullptr;

	// 부채꼴 관련 멤버 변수들
	ComPtr<ID3D12Resource>	 m_fanVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_fanVertexBufferView;
	ComPtr<ID3D12Resource>	 m_fanIndexBuffer;
	D3D12_INDEX_BUFFER_VIEW	 m_fanIndexBufferView;
	ComPtr<ID3D12Resource>	 m_fanConstantBuffer;		   // 부채꼴용 상수 버퍼 추가
	UINT8*					 m_pFanCbvDataBegin = nullptr; // 부채꼴 상수 버퍼 데이터 포인터
	UINT					 m_fanIndexCount = 0;		   // 부채꼴 인덱스 개수

	// 부채꼴 설정 값들
	float m_fanAngle = (2.0f * DirectX::XM_PI) / 4.0f; // 120도 부채꼴
	float m_fanRadius = 3.0f;
	int	  m_fanSegments = 15;

	// 지휘 영역 표시 멤버 변수들
	ComPtr<ID3D12Resource>	 m_commandAreaVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_commandAreaVertexBufferView;
	ComPtr<ID3D12Resource>	 m_commandAreaIndexBuffer;
	D3D12_INDEX_BUFFER_VIEW	 m_commandAreaIndexBufferView;
	ComPtr<ID3D12Resource>	 m_commandAreaConstantBuffer;
	ConstantBuffer			 m_commandAreaConstantBufferData;
	UINT8*					 m_pCommandAreaCbvDataBegin = nullptr;

	// 지휘 영역 설정 값들
	float m_commandAreaWidth = 4.0f;	 // 직사각형 가로 크기
	float m_commandAreaHeight = 3.0f;	 // 직사각형 세로 크기
	float m_commandAreaDistance = 10.0f; // 플레이어로부터의 거리

public:
	virtual void Initialize(ID3D12Device* device) override;
	virtual void Update(float deltaTime) override;
	virtual void Render(ID3D12GraphicsCommandList* cmdList, DirectX::XMMATRIX view, DirectX::XMMATRIX projection)
		override;

public:
	// 부채꼴 관련 함수들
	void RenderFan(
		ID3D12GraphicsCommandList* cmdList, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection
	);
	void InitializeFan(ID3D12Device* device);


	// 지휘 영역 관련 함수들
	void RenderCommandArea(
		ID3D12GraphicsCommandList* cmdList, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection
	);
	void InitializeCommandArea(ID3D12Device* device);


private:
	// 등속도 운동
	void UniformVelocity(const float deltaTime);

	// 등가속도 운동
	void UniformAcceleration(const float deltaTime);

private:
	void UpdateInput(const float deltaTime);
	void UpdatePos(const float deltaTime);
	void SendMovePacket();

	// 와이어프레임 초기화
	void InitializeWireFrame(ID3D12Device* device);

	// 플레이어가 바라보는 방향의 바닥 위치 계산
	DirectX::XMFLOAT3 CalculateGroundTargetPosition() const;
};
