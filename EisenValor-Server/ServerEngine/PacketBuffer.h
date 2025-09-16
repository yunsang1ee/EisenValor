#pragma once

#include "PacketHeader.h"

namespace ServerEngine {
	class PacketBuffer {
	private:
		std::vector<char>	m_buffer;
		uint32				m_capacity;
		uint32				m_dataSize;
		
	public:
		explicit PacketBuffer(const PacketHeader& header);
		~PacketBuffer();

	public:
		void	Append(const BYTE* const src, const uint32 size) noexcept;
		char*	GetBuffer() noexcept { return m_buffer.data(); }
		uint32	GetDataSize() const noexcept { return m_dataSize; }
		uint32	GetCapacity() const noexcept { return m_capacity; }
	};
}


