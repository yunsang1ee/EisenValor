#pragma once

namespace LobbyServerEngine {
	class SocketUtils {
	public:
		static LPFN_CONNECTEX		ConnectEx;

	public:
		static bool Init();
		static SOCKET CreateSocket();
		static bool BindWindowsFunction(SOCKET socket, GUID guid, LPVOID* fn);
	};
}


