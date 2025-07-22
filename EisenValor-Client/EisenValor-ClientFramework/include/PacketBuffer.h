#pragma once

namespace NetBridge {
	class PacketBuffer {
	private:
		std::vector<char>	m_buffer;
		uint64				m_capacity;
		uint64				m_dataSize;

	public:
		explicit PacketBuffer(const uint64 capacity);
		~PacketBuffer();

	public:
		void	Append(const char* const src, const uint64 size);
		char*	GetBuffer() noexcept { return m_buffer.data(); }
		uint64	GetCapacity() const noexcept { return m_capacity; }
	};
}