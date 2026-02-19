#pragma once

namespace ServerEngine {
	class IOCore {
	public:
		IOCore();
		virtual ~IOCore()=default;

	public:
		virtual bool Init(const SessionFactoryFunc func);
		virtual bool StartAccept();
		virtual void Run() abstract;
		virtual void Shutdown() abstract;

	protected:
		SOCKET	CreateSocket(const DWORD flags);
		void	DistributeReservedTask();
		void	FlushTaskQueue();
		inline int	GetPeerName(const SOCKET clientSocket, sockaddr* name, int* nameLen) {  return getpeername(clientSocket, name, nameLen);}

	protected:
		SOCKET									m_listenSocket;
		SOCKADDR_IN								m_serverAddress;
		uint16									m_workerThreadCount;
		SessionFactoryFunc						m_sessionFactoryFunc;

	};
}