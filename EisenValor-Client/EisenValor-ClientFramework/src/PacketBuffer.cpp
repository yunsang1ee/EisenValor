#include "stdafxClientFramework.h"
#include "PacketBuffer.h"

NetBridge::PacketBuffer::PacketBuffer(const uint64 capacity) : m_capacity(capacity)
{
	m_buffer.resize(m_capacity);
}

NetBridge::PacketBuffer::~PacketBuffer() {}