#pragma once

#include "NetBridgeDefines.h"

#include <iostream>
#include <functional>
#include <print>
#include <numeric>

#include <WS2tcpip.h>
#include <MSWSock.h>
#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")

#include "NetBridgeTypes.h"
#include "NetBridgeStructs.h"

#include "NetBridgeEnums.h"
#include "../../EisenValor-Server/Server/TestPackets.h"

#include "flatbuffers\\flatbuffers.h"

#include "Enums_generated.h"
#include "Structs_generated.h"
#include "Tables_generated.h"

#include "NetworkManager.h"
#include "PacketHandler.h"

