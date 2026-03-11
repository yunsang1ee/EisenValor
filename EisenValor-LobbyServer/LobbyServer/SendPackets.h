#pragma once

namespace LobbyServer {
	std::shared_ptr<LobbyServerEngine::PacketBuffer> Make_SC_LOGIN_SUCCESS_PACKET(const uint32 id, const std::string_view nickName);
}
