#pragma once
#include "InputDef.h"

class IInputGlobal : public IGlobal
{
public:
	virtual void Initialize() = 0;
	virtual void BeforeUpdate() = 0;
	virtual void AfterUpdate() = 0;

	virtual void OnInputState(InputCode code, bool isPressed, bool isUp) = 0;

	virtual void OnMouseMove(int x, int y) noexcept = 0;
	virtual void OnWheelScroll(int delta) noexcept = 0;

	virtual bool		 GetInputDown(InputCode code) const noexcept = 0;
	virtual bool		 GetInput(InputCode code) const noexcept = 0;
	virtual bool		 GetInputUp(InputCode code) const noexcept = 0;
	virtual int			 GetWheelScroll() const noexcept = 0;
	virtual DX::XMFLOAT2 GetMousePosition() const noexcept = 0;
	// virtual int GetMousePosition() const noexcept = 0;

	// 마우스 델타 값 반환
	virtual DX::XMFLOAT2 GetMouseDelta() const noexcept = 0;
};
