#pragma once

#include "JobQueue.h"

namespace ServerEngine {
	class IRoom;
	class IOCoreTest;

	class WorkerThread : public JobQueue {
	public:
		explicit WorkerThread(const GameWorldTestFactoryFunc func, std::unique_ptr<IOCoreTest>&& ioCore);
		virtual ~WorkerThread();

	public:
		bool Init();
		void Run(const std::stop_token st);

	public:
		void EnterWorld(std::shared_ptr<Session> session);
		void Register(std::shared_ptr<Session> session);

	public:
		IOCoreTest* GetIoCore() const { return m_ioCore.get(); }

	private:
		std::unique_ptr<IOCoreTest>					m_ioCore;
		std::map<uint32, std::unique_ptr<IRoom>>	m_worlds;
		// TODO: SessionPool 필요
		const GameWorldTestFactoryFunc				m_func;
	};
}