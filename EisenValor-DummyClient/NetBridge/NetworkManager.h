#pragma once

#include "../../EisenValor-Server/ServerEngine/Singleton.hpp"

namespace NetBridge {
	class NetworkManager : public Singleton<NetworkManager> {
		SINGLETON(NetworkManager)
	private:
		SOCKET m_socket;

	public:
		[[nodiscard("DO NOT IGNORE RETURN VALUE.")]]
		bool Init(const std::string_view ip = "127.0.0.1", const uint16 port = 7777);

		void ProcessIO();

		void Terminate();

		template<typename Packet>
		void Send(Packet&& sendPkt) noexcept
		{
			const int32 sendBytes = send(m_socket, reinterpret_cast<char*>(&sendPkt), sizeof(std::decay_t<Packet>), 0);
			if(SOCKET_ERROR == sendBytes) {
				const int32 errCode = WSAGetLastError();
				if(WSAEWOULDBLOCK == errCode) {
					std::cout << "WSAEWOULDBLOCK" << std::endl;
					return;
				}
				else {
					std::println("Send Error = {}", errCode);
					return;
				}
			}
#ifdef  _DEBUG
			std::println("Send!, sendBytes = {}", sendBytes);
#endif 
		}
	};
}

template<typename Packet>
static inline void SendPacket(Packet&& sendPkt) noexcept
{
	MANAGER(NetBridge::NetworkManager)->Send(std::forward<Packet>(sendPkt));
}