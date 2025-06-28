#pragma once
#include "ITimerGlobal.h"

class TimerGlobal : public GlobalMakerBase<TimerGlobal, ITimerGlobal> {

public:
	void Initialize() override;
	void Update() override;
};

