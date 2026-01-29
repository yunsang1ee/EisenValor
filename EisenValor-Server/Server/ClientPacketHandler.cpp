#include "pch.h"
#include "ClientPacketHandler.h"

void Server::ClientPacketHandler::Init() noexcept
{
	for(auto& packetHandlerFunc : PacketHandlerFuncs)
		packetHandlerFunc = ClientPackets::Handle_INVALID_PACKET;

	// =================
	//		세션
	// =================
#pragma region SESSION_PACKETS
	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_PONG_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_PONG_PACKET>(ClientPackets::Handle_CS_PONG_PACKET, session, buffer); };
	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_CHAT_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_CHAT_PACKET>(ClientPackets::Handle_CS_CHAT_PACKET, session, buffer); };
#pragma endregion

	// =================
	//		로그인
	// =================
#pragma region LOGIN_PACKETS
	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_LOGIN_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_LOGIN_PACKET>(ClientPackets::Handle_CS_LOGIN_PACKET, session, buffer); };
#pragma endregion
	// =================
	//		로비
	// =================
#pragma region LOBBY_PACKETS
	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_ENTER_GAME_LOBBY_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_ENTER_GAME_LOBBY_PACKET>(ClientPackets::Handle_CS_ENTER_GAME_LOBBY_PACKET, session, buffer); };
	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_LEAVE_GAME_LOBBY_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_LEAVE_GAME_LOBBY_PACKET>(ClientPackets::Handle_CS_LEAVE_GAME_LOBBY_PACKET, session, buffer); };
	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_MAKE_GAME_ROOM_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_MAKE_GAME_ROOM_PACKET>(ClientPackets::Handle_CS_MAKE_GAME_ROOM_PACKET, session, buffer); };
#pragma endregion
	// =================
	//		룸
	// =================
#pragma region ROOM_PACKETS
	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_JOIN_GAME_ROOM_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_JOIN_GAME_ROOM_PACKET>(ClientPackets::Handle_CS_JOIN_GAME_ROOM_PACKET, session, buffer); };
	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_LEAVE_GAME_ROOM_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_LEAVE_GAME_ROOM_PACKET>(ClientPackets::Handle_CS_LEAVE_GAME_ROOM_PACKET, session, buffer); };
	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_CHANGE_TEAM_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_CHANGE_TEAM_PACKET>(ClientPackets::Handle_CS_CHANGE_TEAM_PACKET, session, buffer); };
	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_ADD_BOT_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_ADD_BOT_PACKET>(ClientPackets::Handle_CS_ADD_BOT_PACKET, session, buffer); };
	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_READY_GAME_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_READY_GAME_PACKET>(ClientPackets::Handle_CS_READY_GAME_PACKET, session, buffer); };
	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_START_GAME_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_START_GAME_PACKET>(ClientPackets::Handle_CS_START_GAME_PACKET, session, buffer); };
	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_COMPLETE_LOADING_GAME_WORLD_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_COMPLETE_LOADING_GAME_WORLD_PACKET>(ClientPackets::Handle_CS_COMPLETE_LOADING_GAME_WORLD_PACKET, session, buffer); };
#pragma endregion
	// =================
	//		월드
	// =================
#pragma region WORLD_PACKETS
	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_MOVE_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_MOVE_PACKET>(ClientPackets::Handle_CS_MOVE_PACKET, session, buffer); };
	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_PLAYER_ATTACK_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_PLAYER_ATTACK>(ClientPackets::Handle_CS_PLAYER_ATTACK_PACKET, session, buffer); };
	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_CHANGE_PLAYER_STANCE_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_CHANGE_PLAYER_STANCE_PACKET>(ClientPackets::Handle_CS_CHANGE_PLAYER_STANCE_PACKET, session, buffer); };
	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_PLAYER_FAKE_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_PLAYER_FAKE_PACKET>(ClientPackets::Handle_CS_PLAYER_FAKE_PACKET, session, buffer); };
	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_CHANGE_CAMERA_TARGET_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_CHANGE_CAMERA_TARGET_PACKET>(ClientPackets::Handle_CS_CHANGE_CAMERA_TARGET_PACKET, session, buffer); };
	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_SHOW_PLAYER_ATTACK_DIR_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_SHOW_PLAYER_ATTACK_DIR_PACKET>(ClientPackets::Handle_CS_SHOW_PLAYER_ATTACK_DIR_PACKET, session, buffer); };
#pragma endregion
	// =================
	//		테스트
	// =================
#pragma region TEST_PACKETS
#ifndef ENABLE_LOBBY
	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::TEST_CS_ENTER_GAME_WORLD_PACKET)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_ENTER_GAME_WORLD_PACKET>(ClientPackets::Handle_CS_ENTER_GAME_WORLD_PACKET, session, buffer); };
#endif // DEVELOP
#pragma endregion

	LOG_INFO("ClientPacketHandler Init");
}

bool Server::ClientPacketHandler::HandlePacket(const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) noexcept
{
	const PacketHeader packetHeader = *reinterpret_cast<const PacketHeader*>(buffer);
	return std::invoke(PacketHandlerFuncs[packetHeader.packetType], session, buffer + sizeof(PacketHeader));
}

std::shared_ptr<ServerEngine::PacketBuffer> Server::ClientPacketHandler::MakePacketBuffer(const PACKET_TYPE packetType, const flatbuffers::DetachedBuffer& packetData) noexcept
{
	const uint16 packetSize = static_cast<uint16>(sizeof(PacketHeader) + (packetData.size()));
	const PacketHeader header{ static_cast<uint16>(packetType), packetSize };

	// TODO: PacketBufferPoolManager로부터 packetBuffer 받아오기

	const auto packetBuffer = ServerEngine::ObjectPool<ServerEngine::PacketBuffer>::MakeShared(header);
	packetBuffer->Append(packetData.data(), packetSize - sizeof(PacketHeader));
	return packetBuffer;
}
