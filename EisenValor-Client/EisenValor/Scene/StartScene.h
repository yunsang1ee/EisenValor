#pragma once
#include <Scene.h>

class StartScene final : public Scene
{
protected:
	void OnRegisterCustomComponents() override;

public:
	void SetLobbyAddress(std::string address) { m_lobbyAddress = std::move(address); }
	void OnStartImpl() override;
	void OnEndImpl() override;

private:
	std::string m_lobbyAddress = "127.0.0.1";
};
