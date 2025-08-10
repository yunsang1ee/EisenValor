#pragma once

#include "Player.h"

class LocalPlayer : public Player
{
private:
	bool sendFlag{false};
	std::chrono::high_resolution_clock::time_point lastSend;

public:
	virtual void Update(float deltaTime) override;

private:
	// 등속도 운동
	void UniformVelocity(const float deltaTime);
	
	// 등가속도 운동
	void UniformAcceleration(const float deltaTime);

	private:
	void UpdateInput(const float deltaTime);
};
