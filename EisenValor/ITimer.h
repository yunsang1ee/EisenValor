#pragma once
#include "ISystem.h"

class ITimer : public IGlobal {
public:
	virtual void Initialize() = 0;
	virtual void Update() = 0;

};

