#pragma once

class ITimerGlobal : public IGlobal {
public:
	virtual void Initialize() = 0;
	virtual void Update() = 0;
	virtual float GetRuntime() const = 0;
	virtual float GetFPS() const = 0;

	// Loop
	virtual float GetDeltaTime() const = 0;
	virtual void SetTargetFPS(uint32_t) = 0;

	// Fixed
	virtual bool ShouldFixedUpdate() = 0;
	virtual float GetFixedDeltaTime() const = 0;
	virtual void SetFixedFPS(uint32_t) = 0;

};

