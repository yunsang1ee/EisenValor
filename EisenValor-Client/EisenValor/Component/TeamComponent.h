#pragma once
#include <IComponent.h>
#include <Packets/Enums_generated.h>

class TeamComponent : public ComponentBase<TeamComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "TeamComponent"; }
	TeamComponent() = default;

	void SetTeamType(FB_ENUMS::TEAM_TYPE teamType) { m_teamType = teamType; }
	FB_ENUMS::TEAM_TYPE GetTeamType() const { return m_teamType; }

private:
	FB_ENUMS::TEAM_TYPE m_teamType = FB_ENUMS::TEAM_TYPE_OFFENSE;
};
