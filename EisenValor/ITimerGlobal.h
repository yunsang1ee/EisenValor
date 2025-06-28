#pragma once
#include "IGlobal.h"

class ITimerGlobal : public IGlobal {
public:
	virtual void Initialize() = 0;
	virtual void Update() = 0;

};

