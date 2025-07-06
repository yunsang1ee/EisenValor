#pragma once

#include "SendBuffer.h"

using PacketHandlerFunc = bool(*)(const SOCKET&, const char* const, const PacketHeader&);

extern inline constinit std::array<PacketHandlerFunc, std::numeric_limits<uint16>::max()+1> PacketHandlerFuncs{};

enum class PACKET_TYPE : uint16 {
	CS_CHAT = 1,
	SC_CHAT = 2,

	END
};

bool Handle_Invalid(const SOCKET& socket, const char* const buffer, const PacketHeader& header);
bool Handle_CS_CHAT_PACKET(const SOCKET& socket, const FB_TABLES::CS_CHAT_PACKET& recvPkt);

namespace NetBridge {
	class SendBuffer;

	class ServerPacketHandler {
	private:
		ServerPacketHandler() = delete;
		~ServerPacketHandler() = delete;
		ServerPacketHandler(const ServerPacketHandler&) = delete;
		ServerPacketHandler& operator= (const ServerPacketHandler&) = delete;
		ServerPacketHandler(ServerPacketHandler&&) noexcept = delete;
		ServerPacketHandler& operator= (ServerPacketHandler&&) noexcept = delete;

	public:
		static void Init() noexcept
		{
			for(auto& packetHandlerFunc : PacketHandlerFuncs)
				packetHandlerFunc = Handle_Invalid;

			PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_CHAT)] = [] (const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool { return HandlePacket<FB_TABLES::CS_CHAT_PACKET>(Handle_CS_CHAT_PACKET, socket, buffer, header); };
		}

		static inline bool HandlePacket(const SOCKET& socket, const char* const buffer, const PacketHeader& packetHeader)
		{
			return std::invoke(PacketHandlerFuncs[packetHeader.packetType], socket, buffer, packetHeader);
		}

		template<typename PacketType, typename HandleFunc>
		static bool HandlePacket(HandleFunc handleFunc, const SOCKET& socket, const char* const buffer, const PacketHeader& packetHeader)
		{
			const PacketType* const packet = flatbuffers::GetRoot<PacketType>(buffer);
			return handleFunc(socket, *packet); 
		}

		// 패킷 만드는 부분
		template<typename PacketFunc, typename... Args>
		static flatbuffers::DetachedBuffer MakePacket(PacketFunc func, Args&&... args)
		{
			flatbuffers::FlatBufferBuilder builder;
			auto offset = func(builder, std::forward<Args>(args)...);
			builder.Finish(offset);
			return builder.Release();
		}

		template<typename T>
		struct PacketArgTraits;

		// =========================
		// CS_CHAT_PACKET
		// =========================
		template<>
		struct PacketArgTraits<struct FB_TABLES::CS_CHAT_PACKET> {
			using ArgTypes = std::tuple<std::string_view>;
			template<typename... Args>
			static constexpr bool ValidArgs = sizeof...(Args) == 1 && (std::convertible_to<Args, std::string_view> && ...);
		};

		template<typename PacketTag, typename... Args>
		static constexpr bool is_valid_packet_args_v = PacketArgTraits<PacketTag>::template ValidArgs<Args...>;

		template<typename... Args>
		[[nodiscard("반환값 절대 무시하지 마세요.")]]
		static flatbuffers::DetachedBuffer Make_CS_CHAT_PACKET(Args&&... args)
		{
			static_assert(is_valid_packet_args_v<FB_TABLES::CS_CHAT_PACKET, Args...>, "CS_CHAT_PACKET requires exactly one std::string_view argument");
			static_assert(sizeof...(Args) == 1, "CS_CHAT_PACKET expects exactly 1 argument");
			static_assert((std::convertible_to<Args, std::string_view> && ...), "All arguments must be convertible to std::string_view");
			return MakePacket(FB_TABLES::CreateCS_CHAT_PACKETDirect, std::forward<Args>(args)...);
		}

		// =========================
		// CS_CHAT_PACKET
		// =========================
		static std::shared_ptr<NetBridge::SendBuffer> MakeSendBuffer(const PACKET_TYPE packetType, const flatbuffers::DetachedBuffer& packetData)
		{
			const uint32 packetSize = static_cast<uint32>(sizeof(PacketHeader) + (packetData.size()));
			auto sendBuffer = std::make_shared<NetBridge::SendBuffer>(packetSize);
			PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->GetBuffer());
			header->packetType = static_cast<uint16>(packetType);
			header->packetSize = packetSize;
			memcpy_s(&header[1], sendBuffer->GetCapacity() - sizeof(PacketHeader), packetData.data(), packetData.size());
			return sendBuffer;
		}
	};
}