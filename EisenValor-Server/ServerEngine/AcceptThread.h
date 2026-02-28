#pragma once

namespace ServerEngine {
	class WorkerThread;
#ifdef  MODERN_CODE


	class AcceptThread {
	public:
		AcceptThread();
		~AcceptThread();

	public:
		AcceptThread(const AcceptThread&) = delete;
		AcceptThread(AcceptThread&&) = delete;
		AcceptThread& operator=(const AcceptThread&) = delete;
		AcceptThread& operator=(AcceptThread&&) = delete;

	public:
		bool Init(const SessionFactoryFunc func, const uint16 port, const DWORD listenSocketFlags);
		void Run(const std::stop_token st);

	private:
		void SetSocketOptions(SOCKET& socket);

	private:
		SOCKET					m_listenSocket;
		SOCKADDR_IN				m_serverAddress;
		WorkerThread*			m_workerThread;
		SessionFactoryFunc		m_func;

		friend class ServerEngineCore;
	};
#endif //  MODERN_CODE

}
