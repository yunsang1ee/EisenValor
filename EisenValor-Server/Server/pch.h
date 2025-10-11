#pragma once
#pragma warning(disable: 4819)
#ifdef _DEBUG
#pragma comment(lib, "ServerEngine\\Debug\\ServerEngine_Debug.lib")
#else
#pragma comment(lib, "ServerEngine\\Release\\ServerEngine_Release.lib")
#endif

#include "ServerEnginePch.h"
#include "flatbuffers\\flatbuffers.h"

#include "Enums_generated.h"
#include "Structs_generated.h"
#include "Tables_generated.h"
#include "ServerTypes.h"
#include "ServerEnums.h"
#include "ServerStructs.h"
#include "ServerGlobalFunc.h"

#include "ClientPacketHandler.h"
#include "ServerPackets.h"
#include "GameObjectFactory.h"

namespace Server {
	class ClientSession;
}

std::shared_ptr<Server::ClientSession> MakeClientSessionFunc();