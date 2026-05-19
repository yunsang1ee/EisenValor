#pragma once

#include "WorkerThread.h"

namespace GameServerEngine {
	class IRoom;

	class GameWorldThread : public WorkerThread {
	public:
		explicit GameWorldThread(const WORKER_THREAD_TYPE type, std::unique_ptr<IOCore>&& ioCore, const GameWorldFactoryFunc worldFunc);
		virtual ~GameWorldThread();

	public:
		virtual bool Init(const SessionFactoryFunc func, const uint16 port) override final;
		virtual void Run(const std::stop_token st) override final;

	public:
		void CreateWorld(const uint16 worldID, const std::unordered_map<uint32, GameWorldParticipantInfo>& info);
		void RequestDestroyWorld(const uint16 worldID);
		void EnterWorld(std::shared_ptr<Session> session);
		IRoom* FindGameWorld(const uint16 worldID);
		uint32 GetWorldCount() const { return m_worldCount.load(std::memory_order_relaxed); }
		uint64 GetLoadScore() const;

	private:
		void ProcessPendingDestroyWorlds();

	private:
		const GameWorldFactoryFunc						m_worldFunc;
		std::map<uint32, std::unique_ptr<IRoom>>		m_worlds;
		std::unordered_set<uint16>						m_pendingDestroyWorldIds;
		std::atomic_uint32_t							m_worldCount;
	};
}


