#pragma once

#include <Scene.h>

class LoadingScene : public Scene
{
protected:
	void OnRegisterCustomComponents() override;

public:
	void OnStartImpl() override;
	void OnEndImpl() override;
};
