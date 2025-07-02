#pragma once

#include <WS2tcpip.h>
#include <MSWSock.h>

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")

#include <iostream> 
#include <string>
#include <string_view>

#include "../../EisenValor-Server/ServerEngine/ServerEnginePch.h"
#include "../../EisenValor-Server/Server/TestPackets.h"

#ifdef _DEBUG
#pragma comment(lib, "../../EisenValor-Server/Libraries/lib/ServerEngine/Debug/ServerEngine_Debug.lib")
#else
#pragma comment(lib, "../../EisenValor-Server/Libraries/lib/ServerEngine/Release/ServerEngine_Release.lib")
#endif
