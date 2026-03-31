#pragma once

#include "PacketHandler.h"	

namespace LobbyServer {
	class ClientPacketHandler final : public LobbyServerEngine::PacketHandler {
	public:
		virtual void Init() override final;

	public:
#pragma region LOGIN_PACKETS
		static bool Handle_CL_LOGIN_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CL_LOGIN_PACKET& recvPkt);
#pragma endregion

#pragma region LOBBY_PACKETS
		static bool Handle_CL_ENTER_GAME_LOBBY_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CL_ENTER_GAME_LOBBY_PACKET& recvPkt);
		static bool Handle_CL_LEAVE_GAME_LOBBY_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CL_LEAVE_GAME_LOBBY_PACKET& recvPkt);
		static bool Handle_CL_MAKE_GAME_ROOM_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CL_MAKE_GAME_ROOM_PACKET& recvPkt);
#pragma endregion

#pragma region ROOM_PACKETS
		static bool Handle_CL_ENTER_GAME_ROOM_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CL_ENTER_GAME_ROOM_PACKET& recvPkt);
		static bool Handle_CL_LEAVE_GAME_ROOM_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CL_LEAVE_GAME_ROOM_PACKET& recvPkt);
		static bool Handle_CL_CHANGE_TEAM_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CL_CHANGE_TEAM_PACKET& recvPkt);
		static bool Handle_CL_ADD_BOT_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CL_ADD_BOT_PACKET& recvPkt);
		static bool Handle_CL_REMOVE_BOT_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CL_REMOVE_BOT_PACKET& recvPkt);
		static bool Handle_CL_READY_GAME_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CL_READY_GAME_PACKET& recvPkt);
		static bool Handle_CL_START_GAME_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CL_START_GAME_PACKET& recvPkt);
		static bool Handle_CL_CHAT_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CL_CHAT_PACKET& recvPkt);
#pragma endregion


	};
}


