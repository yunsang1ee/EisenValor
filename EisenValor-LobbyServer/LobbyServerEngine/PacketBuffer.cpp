#include "pch.h"
#include "PacketBuffer.h"

LobbyServerEngine::PacketBuffer::PacketBuffer(const PacketHeader& header)
	:m_capacity(header.packetSize), m_dataSize(0)
{
	// std::cout << "PacketBuffer" << std::endl;
	//m_buffer.resize(m_capacity);
	//memcpy(m_buffer.data(), (char*)&header, sizeof(PacketHeader));
	//const uint32 size{ static_cast<uint32>(sizeof(PacketHeader)) };
	//m_dataSize += size;

	m_buffer.resize(m_capacity);

	PacketHeader* destHeader = reinterpret_cast<PacketHeader*>(m_buffer.data());

	destHeader->packetType = header.packetType;
	destHeader->packetSize = header.packetSize;

	m_dataSize = sizeof(PacketHeader);
}

LobbyServerEngine::PacketBuffer::~PacketBuffer()
{
	// std::cout << "~PacketBuffer" << std::endl;
}

void LobbyServerEngine::PacketBuffer::Append(const BYTE* const src, const uint32 size)
{
	memcpy_s(&m_buffer[m_dataSize], m_capacity - m_dataSize, src, size);
	m_dataSize += size;
}