#pragma once
#include <Scene.h>
class SampleScene final : public Scene
{
protected:
	void OnRegisterCustomComponents();
	void OnRegisterCustomSceneComponentDecoders() override;

public:
	void OnStartImpl() override;
	void CreateSceneObjects();
	void OnEndImpl() override;
};
