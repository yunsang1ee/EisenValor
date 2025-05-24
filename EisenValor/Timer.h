#pragma once
#include "ITimer.h"

class Timer : public GlobalMakerBase<Timer, ITimer> {

public:
	void Initialize() override;
	void Update() override;
};

