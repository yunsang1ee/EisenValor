#pragma once

#include "Singleton.hpp"

namespace ServerEngine {
	class RIOWorker;

	class RIOCore :public Singleton<RIOCore> {
		SINGLETON(RIOCore)
	private:
		RIO_EXTENSION_FUNCTION_TABLE			m_rioExtfuncTable{};
		SOCKET									m_listenSocket;
		SOCKADDR_IN								m_serverAddress{};
		uint16									m_rioWorkerCnt;
		uint16									m_acceptThreadNum;

		std::vector<std::shared_ptr<RIOWorker>>	m_rioWorkers;
	
	public:
		[[nodiscard("DO NOT IGNORE RETURN VALUE")]] bool Init(SessionFactoryFunc sessionFunc) noexcept;
		[[nodiscard("DO NOT IGNORE RETURN VALUE")]] bool StartAccept() noexcept;
		void			Run() noexcept;
	
	public:
		const auto&		GetRioExtFuncTB() const noexcept { return m_rioExtfuncTable; }
		void			Shutdown();

	private:
		void			DoAcceptLoop() noexcept;

	private:
		void			DistributeReservedTask();
		void			DoTask();
	};
}