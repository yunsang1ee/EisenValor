#pragma once

namespace ServerEngine {
	class IOCore {
	protected:
		SOCKET									m_listenSocket;		
		SOCKADDR_IN								m_serverAddress;	
		uint16									m_workerThreadCount;	
		SessionFactoryFunc						m_sessionFactoryFunc;
	
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
	};
}