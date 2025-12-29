#pragma once
#include <Scene.h>
class SampleScene : public Scene
{
protected:
	void OnRegisterCustomComponents() override;

public:
	void OnStart() override;
	void CreateSceneObjects();
	void OnEnd() override;
};
