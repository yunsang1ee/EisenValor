#pragma once
#include "NetBridgeDefines.h"
#include "NetBridgeEnums.h"

#include "../../EisenValor-Server/ServerEngine/ServerEnginePch.h"
#include "../../EisenValor-Server/Server/TestPackets.h"

#ifdef _DEBUG
#pragma comment(lib, "../../EisenValor-Server/Libraries/lib/ServerEngine/Debug/ServerEngine_Debug.lib")
#else
#pragma comment(lib, "../../EisenValor-Server/Libraries/lib/ServerEngine/Release/ServerEngine_Release.lib")
#endif

#include "flatbuffers\\flatbuffers.h"

#include "Enums_generated.h"
#include "Structs_generated.h"
#include "Tables_generated.h"

#include "NetworkManager.h"
#include "PacketHandler.h"

