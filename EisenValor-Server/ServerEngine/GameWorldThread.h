#pragma once

#include "WorkerThread.h"

namespace ServerEngine {
	class IRoom;

	class GameWorldThread : public WorkerThread {
	public:
		explicit GameWorldThread(const WORKER_THREAD_TYPE type, std::unique_ptr<IOCore>&& ioCore, const GameWorldFactoryFunc worldFunc);
		virtual ~GameWorldThread();

	public:
		virtual bool Init(const SessionFactoryFunc func, const uint16 port) override final;
		virtual void Run(const std::stop_token st) override final;

	public:
		void CreateWorld(const uint16 roomID, const std::unordered_map<uint32, GameWorldParticipantInfo>& info);
		void EnterWorld(std::shared_ptr<Session> session);
		IRoom* FindGameWorld(const uint16 worldID);

	private:
		const GameWorldFactoryFunc					m_worldFunc;
		std::map<uint32, std::unique_ptr<IRoom>>		m_worlds;
	};
}


