#pragma once

namespace ServerEngine {
	class PacketBuffer;
}

namespace ServerPackets {
	// =================
	//		로그인
	// =================
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_LOGIN_SUCCESS_PACKET(const uint32 id, const std::string_view nickName) noexcept;
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_LOGIN_FAIL_PACKET(const std::string_view failMsg) noexcept;

	// =================
	//		로비
	// =================
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_ENTER_GAME_LOBBY_PACKET(const std::vector<RoomInfo>& rooms, const std::vector<std::string_view>& users, const std::vector<uint32>& vecUserID) noexcept;
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_ADD_USER_IN_GAME_LOBBY_PACKET(const std::string_view user, const uint32 id);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_LEAVE_GAME_LOBBY_PACKET();
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_REMOVE_USER_IN_GAME_LOBBY_PACKET(const uint32 id);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_MAKE_GAME_ROOM_PACKET(const RoomInfo& roomInfo);

	// =================
	//		룸
	// =================
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_JOIN_GAME_ROOM_FAIL_PACKET(const std::string_view msg);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_JOIN_GAME_ROOM_SUCCESS_PACKET(const ParticipantInfo& user, const std::vector<ParticipantInfo>& participants);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_LEAVE_GAME_ROOM_PACKET();
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_JOIN_PARTICIPANT_IN_GAME_ROOM_PACKET(const ParticipantInfo& participant);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_LEAVE_PARTICIPANT_IN_GAME_ROOM_PACKET(const uint32 id);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_CHANGE_TEAM_PACKET(const uint32 id, const FB_ENUMS::TEAM_TYPE teamType);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_READY_GAME_PACKET(const uint32 id, const FB_ENUMS::PARTICIPANT_STATE_TYPE stateType);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_LOADING_GAME_WORLD_PACKET();
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_CHANGE_GAME_ROOM_STATE_PACKET(const uint16 id, const FB_ENUMS::ROOM_STATE_TYPE stateType);



	// ==================
	//		월드
	// ==================
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_CHAT_PACKET(const std::string_view msg) noexcept;
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_LOCAL_PLAYER(const uint32 id, const PosInfo& transform, const FB_ENUMS::TEAM_TYPE teamType, const uint32 maxHp, const uint32 currentHp, const uint32 maxStamina, const uint32 currentStamina) noexcept;
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_ADD_OBJ_PACKET(const uint32 id, const FB_ENUMS::GAME_OBJECT_TYPE objType, const FB_ENUMS::TEAM_TYPE teamType, const PosInfo& transform, const uint32 maxHp, const uint32 currentHp, const uint32 maxStamina, const uint32 currentStamina) noexcept;
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_REMOVE_OBJ(const uint32 id) noexcept;
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_MOVE_PACKET(const uint32 id, const PosInfo& transform) noexcept;
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_HIT_PACKET(const uint32 id, const uint32 hp) noexcept;
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_REMANING_GAME_TIME_PACKET(const uint32 remainTime);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_CHANGE_CAMERA_TARGET_PACKET(const uint32 targetID);

	
	// ==================
	//		세션
	// ==================
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_PING_PACKET();
}