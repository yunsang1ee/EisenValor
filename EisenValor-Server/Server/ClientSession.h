#pragma once
#include "Session.h"

#include "TaskQueue.h"
namespace Server {
	namespace Contents {
		class GameRoom;
		class GameWorld;

		class GameLobbyTest;
		class GameRoomTest;
		class GameWorldTest;
	}

#ifdef _USE_IOCP
	class IOCPClientSession : public ServerEngine::IOCP::IOCPSession {
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
		void SetName(const std::string_view name) { m_name = name.data(); }
		void SetGameRoom(std::shared_ptr<Server::Contents::GameRoom> gameRoom) { m_gameRoom = gameRoom; }
		void SetGameWorld(std::shared_ptr<Server::Contents::GameWorld> gameWorld) { m_gameWorld = gameWorld; }

		const std::string& GetName() const { return m_name; }
		std::shared_ptr<Server::Contents::GameRoom> GetGameRoom() const { return m_gameRoom.lock(); }
		std::shared_ptr<Server::Contents::GameWorld> GetGameWorld() const { return m_gameWorld.lock(); }

	private:
		std::string															m_name;				// ClientSession
		std::weak_ptr<Server::Contents::GameRoom>							m_gameRoom;			// ClientSession
		std::weak_ptr<Server::Contents::GameWorld>							m_gameWorld;		// ClientSession
	};
#endif

#ifdef _USE_RIO
#ifdef LEGACY_CODE
	class RIOClientSession : public ServerEngine::RIO::RIOSession {
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
		void SetName(const std::string_view name) { m_name = name.data(); }
		void SetGameRoom(std::shared_ptr<Server::Contents::GameRoom> gameRoom) { m_gameRoom = gameRoom; }
		void SetGameWorld(std::shared_ptr<Server::Contents::GameWorld> gameWorld) { m_gameWorld = gameWorld; }

		const std::string& GetName() const { return m_name; }
		std::shared_ptr<Server::Contents::GameRoom> GetGameRoom() const { return m_gameRoom.lock(); }
		std::shared_ptr<Server::Contents::GameWorld> GetGameWorld() const { return m_gameWorld.lock(); }

	private:
		std::string															m_name;				// ClientSession
		std::weak_ptr<Server::Contents::GameRoom>							m_gameRoom;			// ClientSession
		std::weak_ptr<Server::Contents::GameWorld>							m_gameWorld;		// ClientSession
	};
#endif

#ifdef MODERN_CODE
	class RIOClientSession final : public ServerEngine::PacketSession {
	public:
		RIOClientSession();
		virtual ~RIOClientSession();

	public:
		virtual void OnConnected() override final;
		virtual void OnDisconnected(const std::string_view reason) override final;
		virtual void OnRecvPacket(const std::span<const char>& buf) override final;
		virtual void OnSend(const uint32 bytesTransferred) override final;
		virtual void SendPing() override final;

	public:
		void SetName(const std::string_view name) { m_name = name.data(); }
		const std::string& GetName() const { return m_name; }

		void SetGameWorld(Server::Contents::GameWorldTest* world) { m_gameWorld = world; }
		Server::Contents::GameWorldTest* GetGameWorld() const { return m_gameWorld; }

	private:
		std::string															m_name;			
		Server::Contents::GameWorldTest*									m_gameWorld;
	};

#endif 
#endif

}