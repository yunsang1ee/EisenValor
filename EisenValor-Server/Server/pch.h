#pragma once
// define ENABLE_LOBBY

#define APLLY_LOBBY_SERVER

#pragma warning(disable: 4819)

#include "ServerEnginePch.h"
#include "flatbuffers\\flatbuffers.h"
#include "Enums_generated.h"
#include "Structs_generated.h"
#include "Tables_generated.h"

#include "ServerTypes.h"
#include "ServerEnums.h"
#include "../Packets/Binaries/PacketEnums.h"

#include "ServerStructs.h"
#include "ServerGlobalVariables.h"
#include "ServerGlobalFuncs.h"
#include "GameDataManager.h"
#include "ServerPackets.h"
#include "GameObjectFactory.h"

#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"
#include "DetourCommon.h"
#include "DetourCrowd.h"

