#pragma once

namespace LobbyServerEngine {
	class SocketUtils {
	public:
		static LPFN_CONNECTEX		ConnectEx;

	public:
		static bool Init();
		static SOCKET CreateSocket();
		static bool BindWindowsFunction(SOCKET socket, GUID guid, LPVOID* fn);
		static bool SetUpdateAcceptSocket(SOCKET socket, SOCKET listenSocket);

	private:
		template<typename T>
		static inline bool SetSockOpt(SOCKET socket, int32 level, int32 optName, T optVal)
		{
			return SOCKET_ERROR != ::setsockopt(socket, level, optName, reinterpret_cast<char*>(&optVal), sizeof(T));
		}
	};
}


