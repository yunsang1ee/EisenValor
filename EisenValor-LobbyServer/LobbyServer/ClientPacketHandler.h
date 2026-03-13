#pragma once

#include "PacketHandler.h"	

namespace LobbyServer {
	class ClientPacketHandler final : public LobbyServerEngine::PacketHandler {
	public:
		virtual void Init() override final;

	public:
#pragma region SESSION_PACKETS
		static bool Handle_CS_LOGIN_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CS_LOGIN_PACKET& recvPkt);
#pragma endregion

#pragma region LOBBY_PACKETS
		static bool Handle_CS_ENTER_GAME_LOBBY_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CS_ENTER_GAME_LOBBY_PACKET& recvPkt);
		static bool Handle_CS_LEAVE_GAME_LOBBY_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CS_LEAVE_GAME_LOBBY_PACKET& recvPkt);
		static bool Handle_CS_MAKE_GAME_ROOM_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CS_MAKE_GAME_ROOM_PACKET& recvPkt);
#pragma endregion

#pragma region ROOM_PACKETS
		static bool Handle_CS_ENTER_GAME_ROOM_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CS_ENTER_GAME_ROOM_PACKET& recvPkt);
		static bool Handle_CS_LEAVE_GAME_ROOM_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CS_LEAVE_GAME_ROOM_PACKET& recvPkt);
		static bool Handle_CS_CHANGE_TEAM_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CS_CHANGE_TEAM_PACKET& recvPkt);
		static bool Handle_CS_ADD_BOT_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CS_ADD_BOT_PACKET& recvPkt);
		static bool Handle_CS_REMOVE_BOT_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CS_REMOVE_BOT_PACKET& recvPkt);
		static bool Handle_CS_READY_GAME_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CS_READY_GAME_PACKET& recvPkt);
		static bool Handle_CL_START_GAME_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CL_START_GAME_PACKET& recvPkt);
#pragma endregion


	};
}


