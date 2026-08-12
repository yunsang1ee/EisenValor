#pragma once

#include <IComponent.h>
#include "TextUIComponent.h"


class WorldSceneControllerComponent final : public ComponentBase<WorldSceneControllerComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "WorldSceneControllerComponent"; }

	void OnStart() override;
	void OnUpdate(float deltaTime);
	void SetTeamScores(uint8_t blueScore, uint8_t redScore);

private:
	void CreateTeamScoreUI();
	bool RefreshTeamScoreText();

	HandleOf<TextUIComponent> m_blueTeamScoreTextHandle;
	HandleOf<TextUIComponent> m_redTeamScoreTextHandle;
	uint8_t					  m_blueScore = 0;
	uint8_t					  m_redScore = 0;
	bool					  m_scoreTextDirty = true;
};
