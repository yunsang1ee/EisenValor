#include "stdafxClientFramework.h"
#include "PacketBuffer.h"

NetBridge::PacketBuffer::PacketBuffer(const uint64 capacity)
	:m_capacity(capacity), m_dataSize{0}
{
	m_buffer.resize(m_capacity);
}

NetBridge::PacketBuffer::~PacketBuffer()
{

}

void NetBridge::PacketBuffer::Append(const char* const src, const uint64 size)
{
	memcpy_s(&m_buffer[m_dataSize], m_capacity, src, size);
	m_dataSize += size;
}