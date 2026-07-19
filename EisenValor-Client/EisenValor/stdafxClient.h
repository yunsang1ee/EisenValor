#pragma once

// Common
#include "CommonInclude.h"
#include "CommonUtils.h"

// DirectX
#include "DxCommon.h"
#include "DxUtils.h"

#include "Packets/Enums_generated.h"
#include "Packets/Structs_generated.h"
#include "Packets/Tables_generated.h"

#include "NavMesh/DetourNavMesh.h"
#include "NavMesh/DetourNavMeshQuery.h"
#include "NavMesh/DetourCommon.h"

extern uint16 G_LOBBY_SERVER_PORT;
extern uint16 G_GAME_SERVER_PORT;

//#define APPLY_LOBBY_SERVER