#pragma once

namespace ServerEngine {
	class WorkerThread;

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
		bool Init(const uint16 port, const DWORD listenSocketFlags);
		void Run(const std::stop_token st);

	private:
		SOCKET				m_listenSocket;
		SOCKADDR_IN			m_serverAddress;
		WorkerThread*		m_workerThread;

		friend class ServerEngineCore;
	};
}
