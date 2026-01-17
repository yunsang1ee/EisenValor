#pragma once
#include <Scene.h>
class SampleScene final : public Scene
{
protected:
	void OnRegisterCustomComponents();

public:
	void OnStartImpl() override;
	void CreateSceneObjects();
	void OnEndImpl() override;
};
