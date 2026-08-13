#pragma once
#include <IComponent.h>

class LoginSceneControllerComponent final : public ComponentBase<LoginSceneControllerComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "LoginControllerComponent"; }

	void OnUpdate(float deltaTime);
	void RequestDialog(std::string message, bool isError);

private:
	enum class DialogState
	{
		ReadyToOpen,
		DialogOpen,
		WaitingForResponse
	};

	std::string m_id;
	std::string m_pendingMessage;
	DialogState m_dialogState = DialogState::ReadyToOpen;
	bool m_firstFramePassed = false;
	bool m_pendingMessageIsError = false;
};
