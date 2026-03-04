#pragma once

#define NOMINMAX
#include <numeric>
#include "IPacketHandler.h"

enum class PACKET_TYPE : uint16;

namespace NetBridge
{
class PacketBuffer;

class ServerPacketHandler : public IPacketHandler
{
public:
	using PacketHandlerFunc = bool (*)(const SOCKET&, const char* const, const PacketHeader&);

	ServerPacketHandler() = default;
	~ServerPacketHandler() = default;
	ServerPacketHandler(const ServerPacketHandler&) = delete;
	ServerPacketHandler& operator=(const ServerPacketHandler&) = delete;
	ServerPacketHandler(ServerPacketHandler&&) noexcept = default;
	ServerPacketHandler& operator=(ServerPacketHandler&&) noexcept = default;

public:
	virtual void Init() noexcept final;
	virtual bool HandlePacket(const SOCKET& socket, const char* const buffer, const PacketHeader& packetHeader) final
	{
		return std::invoke(PacketHandlerFuncs[packetHeader.packetType], socket, buffer, packetHeader);
	}

	//template <typename PacketType, typename HandleFunc>
	//static bool HandlePacket(
	//	HandleFunc handleFunc, const SOCKET& socket, const char* const buffer, const PacketHeader& packetHeader
	//)
	//{
	//	const PacketType* const packet = flatbuffers::GetRoot<PacketType>(buffer);
	//	return handleFunc(socket, *packet);
	//}

	template <typename T, typename Func>
	static bool HandlePacket(
		const Func handleFunc, const SOCKET& socket, const char* const buffer, const PacketHeader& header
	)
	{
		const size_t fbSize = header.packetSize - sizeof(PacketHeader);
		flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t*>(buffer), fbSize);

		if (!verifier.VerifyBuffer<T>(nullptr))
		{
			std::println(
				"FlatBuffers Verification Failed! PacketType: {}, Size: {}", header.packetType, header.packetSize
			);
			assert(false && "FLatBuffer Varifier");
			return false;
		}

		auto pktObj = flatbuffers::GetRoot<T>(buffer);
		return handleFunc(socket, *pktObj);
	}

	// 패킷 만드는 부분
	template <typename PacketFunc, typename... Args>
	static flatbuffers::DetachedBuffer Serialization(flatbuffers::FlatBufferBuilder& builder, PacketFunc func, Args&&... args)
	{
		auto offset = func(builder, std::forward<Args>(args)...);
		builder.Finish(offset);
		return builder.Release();
	}

	static std::shared_ptr<NetBridge::PacketBuffer> MakePacketBuffer(
		const PACKET_TYPE packetType, const flatbuffers::DetachedBuffer& packetData
	);

private:
	static std::array<PacketHandlerFunc, std::numeric_limits<uint16>::max() + 1> PacketHandlerFuncs;
};
} // namespace NetBridge