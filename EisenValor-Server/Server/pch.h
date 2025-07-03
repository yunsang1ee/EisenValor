#pragma once

#ifdef _DEBUG
#pragma comment(lib, "ServerEngine\\Debug\\ServerEngine_Debug.lib")
#else
#pragma comment(lib, "ServerEngine\\Release\\ServerEngine_Release.lib")
#endif

#include "ServerEnginePch.h"
#include "TestPackets.h"

namespace Server {
	class ClientSession;
}

std::shared_ptr<Server::ClientSession> MakeClientSessionFunc();