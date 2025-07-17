#pragma once

#include "Enums_generated.h"
#include "Structs_generated.h"
#include "Tables_generated.h"

#include "PacketHeader.h"

namespace ServerEngine {
	class Session;
	class PacketBuffer;
}

enum class PACKET_TYPE : uint16 {
	CS_LOGIN = 1,

	CS_CHAT = 2,
	SC_CHAT = 3,

	END
};

using PacketHandlerFunc = bool(*)(const std::shared_ptr<ServerEngine::Session>&, const char* const, const PacketHeader&);
extern inline constinit std::array<PacketHandlerFunc, std::numeric_limits<uint16>::max() + 1> PacketHandlerFuncs{};

bool Handle_Invalid(const std::shared_ptr<ServerEngine::Session>&, const char* const, const PacketHeader&);
bool Handle_CS_LOGIN_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_LOGIN_PACKET& recvPkt);
bool Handle_CS_CHAT_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_CHAT_PACKET& recvPkt);

class ClientPacketHandler {
private:
	ClientPacketHandler() = delete;
	~ClientPacketHandler() = delete;
	ClientPacketHandler(const ClientPacketHandler&) = delete;
	ClientPacketHandler& operator= (const ClientPacketHandler&) = delete;
	ClientPacketHandler(ClientPacketHandler&&) noexcept = delete;
	ClientPacketHandler& operator= (ClientPacketHandler&&) noexcept = delete;

public:
	// 패킷 받는 부분 
	static void Init() noexcept
	{
		for(auto& packetHandlerFunc : PacketHandlerFuncs)
			packetHandlerFunc = Handle_Invalid;

		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_LOGIN)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer, const PacketHeader& header) -> bool { return HandlePacket<FB_TABLES::CS_LOGIN_PACKET>(Handle_CS_LOGIN_PACKET, session, buffer, header); };
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_CHAT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer, const PacketHeader& header) -> bool { return HandlePacket<FB_TABLES::CS_CHAT_PACKET>(Handle_CS_CHAT_PACKET, session, buffer, header); };
	}

	static inline bool HandlePacket(const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer, const PacketHeader packetHeader)
	{
		return std::invoke(PacketHandlerFuncs[packetHeader.packetType], session, buffer, packetHeader);
	}

	template<typename PacketType, typename HandleFunc>
	static bool HandlePacket(HandleFunc handleFunc, const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer, const PacketHeader packetHeader)
	{
		const PacketType* const packet = flatbuffers::GetRoot<PacketType>(buffer);
		return handleFunc(session, *packet);
	}

	// 패킷 만드는 부분
	template<typename T>
	struct PacketArgTraits;

	template<typename PacketFunc, typename... Args>
	static flatbuffers::DetachedBuffer MakePacket(PacketFunc func, Args&&... args)
	{
		flatbuffers::FlatBufferBuilder builder;
		auto offset = func(builder, std::forward<Args>(args)...);
		builder.Finish(offset);
		return builder.Release();
	}

	static std::shared_ptr<ServerEngine::PacketBuffer> MakePacketBuffer(const PACKET_TYPE packetType, const flatbuffers::DetachedBuffer& packetData)
	{
		const uint16 packetSize = static_cast<uint16>(sizeof(PacketHeader) + (packetData.size()));
		const PacketHeader header{ static_cast<uint16>(packetType), packetSize };
		auto packetBuffer = std::make_shared<ServerEngine::PacketBuffer>(header);
		packetBuffer->Append(packetData.data(), packetSize - sizeof(PacketHeader));
		return packetBuffer;
	}

#pragma region SC_CHAT_PACKET
	//template<>
	//struct PacketArgTraits<struct FB_TABLES::SC_CHAT_PACKET/*패킷명*/> {
	//	using ArgTypes = std::tuple<std::string_view/*패킷 인자*/>;
	//	template<typename... Args>
	//	static constexpr bool ValidArgs = sizeof...(Args) == 1/*패킷 인자 개수*/ && (std::convertible_to<Args, std::string_view/*패킷 인자*/> && ...);
	//};

	//template<typename PacketTag, typename... Args>
	//static constexpr bool is_valid_packet_args_v = PacketArgTraits<PacketTag>::template ValidArgs<Args...>;

	template<typename... Args>
	[[nodiscard("반환값 절대 무시하지 마세요.")]]
	/*Make_패킷명*/
	static flatbuffers::DetachedBuffer Make_SC_CHAT_PACKET(Args&&... args)
	{
		//static_assert(is_valid_packet_args_v<FB_TABLES::SC_CHAT_PACKET/*패킷명*/, Args...>, "SC_CHAT_PACKET requires exactly one std::string_view argument");
		//static_assert(sizeof...(Args) == 1/*패킷 인자 개수*/, "SC_CHAT_PACKET expects exactly 1 argument");
		//static_assert((std::convertible_to<Args, std::string_view/*패킷 인자*/> && ...), "All arguments must be convertible to std::string_view");
		return MakePacket(FB_TABLES::CreateSC_CHAT_PACKETDirect, std::forward<Args>(args)...);
	}
#pragma endregion
};

