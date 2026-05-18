#pragma once
#include <PacketBuffer.cpp>

namespace LobbyServer {

#pragma region SESSION_PACKETS
	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_CS_PONG_PACKET();
#pragma endregion

#pragma region LOGIN_PACKETS
	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_LC_LOGIN_SUCCESS_PACKET(const uint32 id, const std::string_view nickName);
	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_LC_LOGIN_FAIL_PACKET(const std::string_view failMsg);
#pragma endregion

#pragma region LOBBY_PACKETS
	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_LC_ENTER_GAME_LOBBY_SUCCESS_PACKET(const std::vector<RoomInfo>& rooms, const std::vector<std::string_view>& users, const std::vector<uint32>& vecUserID);
	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_LC_ENTER_GAME_LOBBY_FAIL_PACKET(const std::string_view failmsg);
	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_LC_LEAVE_GAME_LOBBY_PACKET();

	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_LC_ENTER_USER_IN_GAME_LOBBY_PACKET(const std::string_view user, const uint32 id);
	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_LC_LEAVE_USER_IN_GAME_LOBBY_PACKET(const uint32 id);
	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_LC_MAKE_GAME_ROOM_PACKET(const RoomInfo& room);
#pragma endregion

#pragma region ROOM_PACKETS
	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_LC_ENTER_GAME_ROOM_FAIL_PACKET(const std::string_view msg);
	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_LC_ENTER_GAME_ROOM_SUCCESS_PACKET(const ParticipantInfo& user, const std::vector<ParticipantInfo>& participants);
	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_LC_LEAVE_GAME_ROOM_PACKET();
	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_LC_ENTER_PARTICIPANT_IN_GAME_ROOM_PACKET(const ParticipantInfo& participant);
	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_LC_LEAVE_PARTICIPANT_IN_GAME_ROOM_PACKET(const uint32 participantID);
	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_LC_CHANGE_TEAM_PACKET(const uint32 participantID, const FB_ENUMS::TEAM_TYPE teamType);
	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_LC_ADD_BOT_PACKET(const ParticipantInfo& bot);
	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_LC_READY_GAME_PACKET(const uint32 participantID, const FB_ENUMS::PARTICIPANT_STATE_TYPE state);
	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_LC_START_GAME_FAIL_PACKET(const std::string_view failMsg);
	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_LS_CREATE_GAME_WORLD_PACKET(const uint16 worldID, const std::vector<ParticipantInfo>& participants);
	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_LC_CONNECT_TO_GAME_SERVER_PACKET	(const uint16 worldID, const std::string_view ip, const uint16 port);
	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_LC_CHAT_PACKET(const uint32 sessionID, const std::string_view msg);
#pragma endregion

}
