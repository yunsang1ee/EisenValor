#pragma once

#include "JobQueue.h"

namespace ServerEngine {
	class IRoom;
	class IOCoreTest;

	class WorkerThread : public JobQueue {
	public:
		explicit WorkerThread(std::unique_ptr<IOCoreTest>&& ioCore);
		virtual ~WorkerThread();

	public:
		bool Init(const GameWorldTestFactory func);
		void Run(const std::stop_token st);

	public:
		void EnterSession(std::shared_ptr<Session> session);

	private:
		std::unique_ptr<IOCoreTest>					m_ioCore;
		std::map<uint32, std::unique_ptr<IRoom>>	m_worlds;
		// TODO: SessionPool 필요
		GameWorldTestFactory						m_func;
	};
}