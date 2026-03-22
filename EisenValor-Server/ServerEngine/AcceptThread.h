#pragma once

namespace GameServerEngine {
	class WorkerThread;
#ifdef  MODERN_CODE

	class AcceptThread {
	public:
		explicit AcceptThread(const SessionFactoryFunc func, const DWORD listenSocketFlags, WorkerThread* const ownerWorker);
		~AcceptThread();

	public:
		AcceptThread(const AcceptThread&) = delete;
		AcceptThread(AcceptThread&&) = delete;
		AcceptThread& operator=(const AcceptThread&) = delete;
		AcceptThread& operator=(AcceptThread&&) = delete;

	public:
		bool Init(const uint16 port);
		void Run(const std::stop_token st);

	public:
		uint16 GetPort() const { return m_port; }

	private:
		void SetSocketOptions(SOCKET& socket);

	private:
		SOCKET						m_listenSocket;
		SOCKADDR_IN					m_serverAddress;
		const SessionFactoryFunc	m_func;
		WorkerThread*				m_ownerWorker;
		uint16						m_port;

		friend class ServerEngineCore;
	};
#endif //  MODERN_CODE

}
