#pragma once

#include <Scene.h>

class ScoreScene final : public Scene
{
public:
	static void SetScores(uint8 redScore, uint8 blueScore);

protected:
	void OnRegisterCustomComponents() override;

public:
	void OnStartImpl() override;
	void OnEndImpl() override;

private:
	static uint8 s_redScore;
	static uint8 s_blueScore;
};
