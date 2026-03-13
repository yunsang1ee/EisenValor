#pragma once

#include "WorkerThread.h"

namespace ServerEngine {
	class IRoom;

	class GameWorldThread : public WorkerThread {
	public:
		explicit GameWorldThread(std::unique_ptr<IOCoreTest>&& ioCore, const GameWorldTestFactoryFunc worldFunc);
		virtual ~GameWorldThread();

	public:
		virtual bool Init(const SessionFactoryFunc func, const uint16 port) override final;
		virtual void Run(const std::stop_token st) override final;

	public:
		void EnterWorld(std::shared_ptr<Session> session);


	private:
		const GameWorldTestFactoryFunc					m_worldFunc;
		std::map<uint32, std::unique_ptr<IRoom>>		m_worlds;
	};
}


