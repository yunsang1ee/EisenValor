#pragma once
#include <IComponent.h>

class LoginControllerComponent final : public ComponentBase<LoginControllerComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "LoginControllerComponent"; }

	void OnUpdate(float deltaTime);

private:
	std::string m_id;
	bool m_firstFramePassed = false;
	bool m_dialogShown = false;
};
