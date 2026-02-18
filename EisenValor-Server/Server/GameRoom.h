#pragma once
#include "TaskQueue.h"

class ClientPacketHandler;

namespace ServerEngine {
	class Objectpool;
}

namespace Server {
	namespace Contents {
		class GameLobby;
		class GameObject;
		class Player;
		class GameWorld;
		class Participant;
		class User;
		class Bot;

		using Users = std::unordered_map<uint32, std::shared_ptr<User>>;
		using Bots = std::unordered_map<uint32, std::shared_ptr<Bot>>;

		// JobQueue에는 GameRoom에서 일어나는 모든 일들이 쌓이게 됨
		// ex) Update(), EnterRoom(), LeaveRoom(), Broadcast() ...
		// GameRoom의 JobQueue를 실행하는 쓰레드는 여러 쓰레드 중, 단 하나만
		class GameRoom : public ServerEngine::TaskQueue {
		private:
			// 세션 아이디 == 유저 아이디 == 플레이어 아이디
			RoomInfo													m_info;

			Users														m_users;
			Bots														m_bots;
			std::shared_ptr<User>										m_host;

			int32														m_offenseCount;
			int32														m_defenseCount;

			std::shared_ptr<GameWorld>									m_gameWorld;
			int32														m_loadingCompletedUserCount;

		public:
			GameRoom() = delete;
			explicit GameRoom(const uint16 roomID);
			~GameRoom();
			GameRoom(const GameRoom&) = delete;
			GameRoom(GameRoom&&) noexcept = delete;
			GameRoom& operator= (const GameRoom&) = delete;
			GameRoom& operator= (GameRoom&&) noexcept = delete;

			friend class GameRoomManager;
			friend class ServerEngine::ObjectPool<GameRoom>;

		public:
			void JoinGameRoom(const std::shared_ptr<ClientSession>& clientSession);
			void LeaveGameRoom(const std::shared_ptr<ClientSession>& clientSession);
			void ReturnToGameRoom(const Users& users, const Bots& bots);
			void Broadcast(std::shared_ptr<ServerEngine::PacketBuffer> packetBuffer);

		public:
			/* 패킷 받아서 처리되는 부분 */
			void Handle_CS_CHANGE_TEAM(const std::shared_ptr<ClientSession>& clientSession);
			void Handle_CS_ADD_BOT(const std::shared_ptr<ClientSession>& clientSession, const FB_ENUMS::TEAM_TYPE teamType);
			void Handle_CS_READY_GAME(const std::shared_ptr<ClientSession>& clientSession);
			void Handle_CS_GAME_START(const std::shared_ptr<ClientSession>& clientSession);
			void Handle_CS_COMPLETE_LOADING_GAME_WORLD(const std::shared_ptr<ClientSession>& clientSession);

#ifndef ENABLE_LOBBY
			void Handle_CS_ENTER_GAME_WORLD(const std::shared_ptr<ClientSession>& clientSession);
#endif // DEVELOP

		public:
			uint16 GetID() const { return m_info.id; }
			const RoomInfo& GetRoomInfo() const { return m_info; }

		private:
			void Init();
			void AddParticipant(std::shared_ptr<Server::Contents::Participant> participant);

		private:
			bool CanStart();
			void CreateWorld();

			friend class GameLobby;
			friend class GameWorld;

		};
	}
}