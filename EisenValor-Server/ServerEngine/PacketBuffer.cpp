#include "pch.h"
#include "PacketBuffer.h"

ServerEngine::PacketBuffer::PacketBuffer(const PacketHeader& header)
	:m_capacity(header.packetSize), m_dataSize(0)
{
	// std::cout << "PacketBuffer" << std::endl;
	m_buffer.resize(m_capacity);
	memcpy(m_buffer.data(), (char*)&header, sizeof(PacketHeader));
	m_dataSize += sizeof(PacketHeader);
}

ServerEngine::PacketBuffer::~PacketBuffer()
{
	// std::cout << "~PacketBuffer" << std::endl;
}

void ServerEngine::PacketBuffer::Append(const BYTE* const src, const uint32 size)
{
	memcpy_s(&m_buffer[m_dataSize], m_capacity- m_dataSize, src, size);
	m_dataSize += size;
}