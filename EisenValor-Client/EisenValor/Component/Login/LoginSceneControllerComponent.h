#pragma once
#include <IComponent.h>

class LoginSceneControllerComponent final : public ComponentBase<LoginSceneControllerComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "LoginControllerComponent"; }

	void OnUpdate(float deltaTime);

	private:
	std::string m_id;
};