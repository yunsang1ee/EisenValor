#pragma once

#include "JobQueue.h"

namespace GameServerEngine {
	class IRoom;
	class IOCore;
	class AcceptThread;

	class WorkerThread : public JobQueue {
	public:
		explicit WorkerThread(const WORKER_THREAD_TYPE type, std::unique_ptr<IOCore>&& ioCore);
		virtual ~WorkerThread();

	public:
		virtual bool Init(const SessionFactoryFunc func, const uint16 port);
		virtual void Run(const std::stop_token st);

	public:
		void Register(std::shared_ptr<Session> session);

	public:
		IOCore* GetIoCore() const { return m_ioCore.get(); }
		uint16 GetPort() const;
		float GetDT() const { return m_dt; }

	protected:
		std::unique_ptr<IOCore>			m_ioCore;
		std::unique_ptr<AcceptThread>	m_acceptThread;
		const WORKER_THREAD_TYPE		m_type;
		float							m_dt;
	};
}
