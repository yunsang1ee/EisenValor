#pragma once
#include "Session.h"
#include "TaskQueue.h"

namespace Server {
	namespace Contents {
		class Player;
		class GameRoom;
		class GameWorld;
	}

	class RIOClientSession : public ServerEngine::RIOSession {
	private:
		std::string															m_name;				// ClientSession
		std::weak_ptr<Server::Contents::GameRoom>							m_gameRoom;			// ClientSession
		std::weak_ptr<Server::Contents::GameWorld>							m_gameWorld;		// ClientSession
		
		
		std::chrono::high_resolution_clock::time_point						m_lastPong;			// Session
		const std::chrono::milliseconds										m_pingInterval;		// Session
		const std::chrono::milliseconds										m_timeoutInterval;	// Session

	public:
		RIOClientSession();
		virtual ~RIOClientSession();

	public:
		virtual void OnConnected() override final;
		virtual void OnDisconnected(const std::string_view reason) override final;
		virtual void ProcessPacket(const std::span<const char>& buffer) override final;
		virtual void OnSend(const uint32 bytesTransferred) override final;

	public:
		void Handle_CS_PONG();

	public:
		void SetName(const std::string_view name) noexcept { m_name = name.data(); }
		void SetGameRoom(std::shared_ptr<Server::Contents::GameRoom> gameRoom) noexcept { m_gameRoom = gameRoom; }
		void SetGameWorld(std::shared_ptr<Server::Contents::GameWorld> gameWorld) noexcept { m_gameWorld = gameWorld; }

		const std::string& GetName() const noexcept { return m_name; }
		std::shared_ptr<Server::Contents::GameRoom> GetGameRoom() const noexcept { return m_gameRoom.lock(); }
		std::shared_ptr<Server::Contents::GameWorld> GetGameWorld() const noexcept { return m_gameWorld.lock(); }

	private:
		void Ping();
	};

	class IOCPClientSession : public ServerEngine::IOCPSession {

	};
}