#pragma once

#include <Scene.h>

class LobbyScene final : public Scene
{
protected:
	void OnRegisterCustomComponents() override;

public:
	void OnStartImpl() override;
	void OnEndImpl() override;
};
