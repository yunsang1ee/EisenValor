#pragma once
#include <IComponent.h>

class PathTracingDemoDoorComponent final : public ComponentBase<PathTracingDemoDoorComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "PathTracingDemoDoorComponent"; }
	void						 OnStart() override;
	void						 OnUpdate(float deltaTime);
	void						 ToggleAutomatic();
	void						 ToggleOpen();
	bool						 IsAutomatic() const { return m_automatic; }
	bool						 IsMoving() const { return m_moving; }
	float						 GetOpenAmount() const { return m_openAmount; }

private:
	void  BeginMove(bool open);
	float m_openAmount = 1.0f;
	float m_startAmount = 1.0f;
	float m_elapsed = 0.0f;
	float m_holdRemaining = 4.0f;
	bool  m_targetOpen = true;
	bool  m_automatic = true;
	bool  m_moving = false;
};
