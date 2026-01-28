#pragma once
#include "Session.h"
#include "TaskQueue.h"

namespace Server {
	namespace Contents {
		class Player;
		class GameRoom;
		class GameWorld;
	}

	// TODO: 공통되는것들 소유하게

	class RIOClientSession : public ServerEngine::RIO::RIOSession {
	private:
		std::string															m_name;				// ClientSession
		std::weak_ptr<Server::Contents::GameRoom>							m_gameRoom;			// ClientSession
		std::weak_ptr<Server::Contents::GameWorld>							m_gameWorld;		// ClientSession

	public:
		RIOClientSession();
		virtual ~RIOClientSession();

	public:
		virtual void OnConnected() override final;
		virtual void OnDisconnected(const std::string_view reason) override final;
		virtual void ProcessPacket(const std::span<const char>& buffer) override final;
		virtual void OnSend(const uint32 bytesTransferred) override final;
		virtual void SendPing() override final;
	
	public:
		void SetName(const std::string_view name) noexcept { m_name = name.data(); }
		void SetGameRoom(std::shared_ptr<Server::Contents::GameRoom> gameRoom) noexcept { m_gameRoom = gameRoom; }
		void SetGameWorld(std::shared_ptr<Server::Contents::GameWorld> gameWorld) noexcept { m_gameWorld = gameWorld; }

		const std::string& GetName() const noexcept { return m_name; }
		std::shared_ptr<Server::Contents::GameRoom> GetGameRoom() const noexcept { return m_gameRoom.lock(); }
		std::shared_ptr<Server::Contents::GameWorld> GetGameWorld() const noexcept { return m_gameWorld.lock(); }
	};

	class IOCPClientSession : public ServerEngine::IOCP::IOCPSession {
	private:
		std::string															m_name;				// ClientSession
		std::weak_ptr<Server::Contents::GameRoom>							m_gameRoom;			// ClientSession
		std::weak_ptr<Server::Contents::GameWorld>							m_gameWorld;		// ClientSession
	
	public:
		IOCPClientSession();
		virtual ~IOCPClientSession();

	public:
		virtual void OnConnected() override final;
		virtual void OnDisconnected(const std::string_view reason) override final;
		virtual void ProcessPacket(const std::span<const char>& buffer) override final;
		virtual void OnSend(const uint32 bytesTransferred) override final;
		virtual void SendPing() override final;

	public:
		void SetName(const std::string_view name) noexcept { m_name = name.data(); }
		void SetGameRoom(std::shared_ptr<Server::Contents::GameRoom> gameRoom) noexcept { m_gameRoom = gameRoom; }
		void SetGameWorld(std::shared_ptr<Server::Contents::GameWorld> gameWorld) noexcept { m_gameWorld = gameWorld; }

		const std::string& GetName() const noexcept { return m_name; }
		std::shared_ptr<Server::Contents::GameRoom> GetGameRoom() const noexcept { return m_gameRoom.lock(); }
		std::shared_ptr<Server::Contents::GameWorld> GetGameWorld() const noexcept { return m_gameWorld.lock(); }
	};
}