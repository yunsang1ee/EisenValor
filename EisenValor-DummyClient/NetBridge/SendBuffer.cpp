#include "pch.h"
#include "SendBuffer.h"

NetBridge::SendBuffer::SendBuffer(const uint64 capacity)
	:m_capacity(capacity)
{
	m_buffer.resize(m_capacity);
}

NetBridge::SendBuffer::~SendBuffer()
{

}

void NetBridge::SendBuffer::Append(const char* const src, const uint64 size)
{
	memcpy_s(&m_buffer[m_dataSize], m_capacity, src, size);
	m_dataSize += size;
}