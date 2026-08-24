#pragma once

#include "Packets/Enums_generated.h"
#include "TextUIComponent.h"
#include <Scene.h>

class ScoreScene final : public Scene
{
public:
	static void SetResult(FB_ENUMS::TEAM_TYPE winningTeam, uint8 blueScore, uint8 redScore);

protected:
	void OnRegisterCustomComponents() override;

public:
	void OnStartImpl() override;
	void OnEndImpl() override;

private:
	void ReturnToRoom();
	void SetStatusText(const std::wstring& message);

	static FB_ENUMS::TEAM_TYPE s_winningTeam;
	static uint8 s_redScore;
	static uint8 s_blueScore;

	HandleOf<TextUIComponent> m_statusTextHandle;
	bool				  m_returnPending = false;
};
