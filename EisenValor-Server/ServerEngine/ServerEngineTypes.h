#pragma once

using BYTE = unsigned char;
using int8 = __int8;
using int16 = __int16;
using int32 = __int32;
using int64 = __int64;
using uint8 = unsigned __int8;
using uint16 = unsigned __int16;
using uint32 = unsigned __int32;
using uint64 = unsigned __int64;

namespace GameServerEngine {
	class Session;
	class Task;
	class TaskQueue;
	class IRoom;

	namespace RIO {
		class RIOSession;
	}

	namespace IOCP {
		class IOCPSession;
	}
}

namespace GameServer {
	class GameWorld;

#ifdef _USE_RIO
	class RIOClientSession;
	class RIOLobbyServerSession;
#endif
}

#ifdef  _USE_RIO
using ClientSessionFactoryFunc = std::function<std::shared_ptr<GameServerEngine::RIO::RIOSession>()>;
using LobbyServerSessionFactoryFunc = std::function<std::shared_ptr<GameServerEngine::RIO::RIOSession>()>;

using SessionFactoryFunc = std::function<std::shared_ptr<GameServerEngine::Session>()>;
#endif

#ifdef	_USE_IOCP
using SessionFactoryFunc = std::function<std::shared_ptr<ServerEngine::IOCP::IOCPSession>()>;
#endif


using GameWorldFactoryFunc = std::function<std::unique_ptr<GameServerEngine::IRoom>()>;
using GameLobbyTestFactoryFunc = std::function<std::unique_ptr<GameServerEngine::IRoom>()>;