#pragma once

namespace GameServerEngine {
	class PacketSession;
}

namespace GameServerEngine {
	class PacketHandler {
		using PacketHandlerFunc = bool (*)(const std::shared_ptr<GameServerEngine::PacketSession>&, const char*);
	public:
		PacketHandler();
		virtual ~PacketHandler();

	public:
		virtual void Init() abstract;

	public:
		bool HandlePacket(const std::shared_ptr<GameServerEngine::PacketSession>& session, const char* const buffer);

	public:
		static bool Handle_INVALID(const std::shared_ptr<GameServerEngine::PacketSession>& session, const char* const buffer) { return false; }

		template<typename PacketType, typename HandleFunc>
		static bool HandlePacket(const HandleFunc handleFunc, const std::shared_ptr<GameServerEngine::PacketSession>& session, const char* const buffer)
		{
			const PacketType* const packet = flatbuffers::GetRoot<PacketType>(buffer);
			return handleFunc(session, *packet);
		}

		template<typename PacketFunc, typename... Args>
		static flatbuffers::DetachedBuffer Serialization(flatbuffers::FlatBufferBuilder& builder, PacketFunc func, Args&&... args)
		{
			auto offset = func(builder, std::forward<Args>(args)...);
			builder.Finish(offset);
			return builder.Release();
		}
		static std::shared_ptr<GameServerEngine::PacketBuffer> MakePacketBuffer(const uint16 packetType, const flatbuffers::DetachedBuffer& packetData);

	protected:
		std::array<PacketHandlerFunc, std::numeric_limits<uint16>::max() + 1> m_packetHandlerFuncs;
	};
}


