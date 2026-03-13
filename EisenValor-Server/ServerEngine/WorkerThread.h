#pragma once

#include "JobQueue.h"

namespace ServerEngine {
	class IRoom;
	class IOCoreTest;
	class AcceptThread;

	class WorkerThread : public JobQueue {
	public:
		explicit WorkerThread(std::unique_ptr<IOCoreTest>&& ioCore);
		virtual ~WorkerThread();

	public:
		virtual bool Init(const SessionFactoryFunc func, const uint16 port);
		virtual void Run(const std::stop_token st);

	public:
		void Register(std::shared_ptr<Session> session);

	public:
		IOCoreTest* GetIoCore() const { return m_ioCore.get(); }

	protected:
		std::unique_ptr<IOCoreTest>					m_ioCore;
		std::unique_ptr<AcceptThread>				m_acceptThread;
	};
}