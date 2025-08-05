#pragma once

#include "Player.h"

class LocalPlayer : public Player
{
private:
	bool sendFlag{false};

public:
	virtual void Update(float deltaTime) override;
};
