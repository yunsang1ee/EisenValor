#pragma once

#include "Session.h"

namespace LobbyServer {
	class ClientSession : public LobbyServerEngine::Session {
	public:
		ClientSession();
		virtual ~ClientSession();
	};
}


