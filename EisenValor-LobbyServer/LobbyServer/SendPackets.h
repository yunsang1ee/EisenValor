#pragma once
#include <PacketBuffer.cpp>

namespace LobbyServer {
#pragma region SESSION_PACKETS
	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_SC_LOGIN_SUCCESS_PACKET(const uint32 id, const std::string_view nickName);
	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_CS_PONG_PACKET();
#pragma endregion

#pragma region LOBBY_PACKETS
	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_SC_ENTER_GAME_LOBBY_SUCCESS_PACKET(const std::vector<RoomInfo>& rooms, const std::vector<std::string_view>& users, const std::vector<uint32>& vecUserID);
	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_SC_LEAVE_GAME_LOBBY_PACKET();

	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_SC_ENTER_USER_IN_GAME_LOBBY_PACKET(const std::string_view user, const uint32 id);
	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_SC_LEAVE_USER_IN_GAME_LOBBY_PACKET(const uint32 id);

#pragma endregion

#pragma region ROOM_PACKETS
	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_SC_ENTER_GAME_ROOM_FAIL_PACKET(const std::string_view msg);
	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_SC_ENTER_GAME_ROOM_SUCCESS_PACKET(const ParticipantInfo& user, const std::vector<ParticipantInfo>& participants);
	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_SC_ENTER_PARTICIPANT_IN_GAME_ROOM_PACKET(const ParticipantInfo& participant);
	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_LS_CREATE_GAME_WORLD_PACKET(const uint16 roomID, const std::vector<ParticipantInfo>& participants);
	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_LC_CONNECT_TO_GAME_SERVER_PACKET(const uint16 worldID, const std::string_view ip, const uint16 port);
#pragma endregion

}
