#pragma once

#include "Player.h"

class LocalPlayer : public Player
{
private:
	bool sendFlag{false};
	std::chrono::high_resolution_clock::time_point lastSend = std::chrono::high_resolution_clock::now();			 	  

public:
	// 부채꼴 관련 함수들
	virtual void Initialize(ID3D12Device* device) override;
	virtual void Update(float deltaTime) override;
	virtual void Render(ID3D12GraphicsCommandList* cmdList, DirectX::XMMATRIX view, DirectX::XMMATRIX projection)
		override;

private:
	// 등속도 운동
	void UniformVelocity(const float deltaTime);
	
	// 등가속도 운동
	void UniformAcceleration(const float deltaTime);

	private:
	void UpdateInput(const float deltaTime);
	void UpdatePos(const float deltaTime);
	void SendMovePacket();

};
