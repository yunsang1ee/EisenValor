#pragma once
#include "Session.h"
#include "TaskQueue.h"

namespace Server {
	namespace Contents {
		class Player;
		class GameRoom;
		class GameWorld;
	}

	class ClientSession : public ServerEngine::Session {
	private:
		std::string															m_name;
		std::weak_ptr<Server::Contents::GameRoom>							m_gameRoom;
		std::weak_ptr<Server::Contents::GameWorld>							m_gameWorld;
		std::chrono::high_resolution_clock::time_point						m_lastPong;

		const std::chrono::milliseconds										m_pingInterval;	
		const std::chrono::milliseconds										m_timeoutInterval;

	public:
		ClientSession();
		virtual ~ClientSession();

	public:
		void SetName(const std::string_view name) noexcept { m_name = name.data(); }
		const std::string& GetName() const noexcept { return m_name; }

		void SetGameRoom(std::shared_ptr<Server::Contents::GameRoom> gameRoom) noexcept { m_gameRoom = gameRoom; }
		std::shared_ptr<Server::Contents::GameRoom> GetGameRoom() const noexcept { return m_gameRoom.lock(); }

		void SetGameWorld(std::shared_ptr<Server::Contents::GameWorld> gameWorld) noexcept { m_gameWorld = gameWorld; }
		std::shared_ptr<Server::Contents::GameWorld> GetGameWorld() const noexcept { return m_gameWorld.lock(); }

	public:
		virtual void OnConnected() override final;
		virtual void OnDisconnected() override final;
		virtual void ProcessPacket(const std::span<const char>& buffer) override final;
		virtual void OnSend(const uint32 bytesTransferred) override final;
		
	public:
		void		 Handle_CS_PONG();
		
	private:
		void Ping();
	};
}