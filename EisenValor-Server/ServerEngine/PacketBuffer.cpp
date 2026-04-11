#include "pch.h"
#include "PacketBuffer.h"

GameServerEngine::PacketBuffer::PacketBuffer(const PacketHeader& header)
	:m_capacity(header.packetSize)
{
	m_buffer.resize(m_capacity);

	PacketHeader* const destHeader = reinterpret_cast<PacketHeader*>(m_buffer.data());

	destHeader->packetType = header.packetType;
	destHeader->packetSize = header.packetSize;

	m_dataSize = sizeof(PacketHeader);
}

GameServerEngine::PacketBuffer::~PacketBuffer()
{
	// std::cout << "~PacketBuffer" << std::endl;
}

void GameServerEngine::PacketBuffer::Append(const BYTE* const src, const uint32 size)
{
	if(m_dataSize + size > m_capacity) {
		std::cout << "PacketBuffer Overflow, DataSize: " << m_dataSize << ", AppendSize: " << size << ", Capacity: " << m_capacity << std::endl;
		return;
	}
	memcpy_s(&m_buffer[m_dataSize], m_capacity- m_dataSize, src, size);
	m_dataSize += size;
}