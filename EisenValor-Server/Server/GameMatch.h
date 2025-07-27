#pragma once
#include "TaskQueue.h"

namespace Server {
	class ClientSession;

	namespace Contents {
		class General;
		class Soldier;

		class GameMatch : public ServerEngine::TaskQueue {
		private:
			uint16										m_id;
			std::mutex									m_mutex;
			std::map<uint32, std::shared_ptr<General>>	m_generals;

		public:
			explicit GameMatch(const uint16 id) :m_id{ id } {}

		public:
			uint16 GetID() const noexcept { return m_id; }

		public:
			void EnterMatch(std::shared_ptr<ClientSession> clientSession) noexcept;
			void LeaveMatch(std::shared_ptr<ClientSession> clientSession) noexcept;
			void BroadcastInMatch(std::shared_ptr<ServerEngine::PacketBuffer> packetBuffer);
			std::shared_ptr<General> GetGeneral(uint32 id) noexcept;

		private:
			void AddGeneral(std::shared_ptr<General>&& general) noexcept;
			void RemoveGeneral(std::shared_ptr<General> general);
		};
	}
}


