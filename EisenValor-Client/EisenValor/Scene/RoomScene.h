#pragma once

#include <Scene.h>

class RoomScene final : public Scene
{
protected:
	void OnRegisterCustomComponents() override;

public:
	void OnStartImpl() override;
	void OnEndImpl() override;
};
