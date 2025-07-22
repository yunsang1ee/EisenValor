#include "stdafxClientFramework.h"
#include "RecvBuffer.h"

NetBridge::RecvBuffer::RecvBuffer(const uint32 bufferSize)
	:m_size(bufferSize),  m_readPos{0}, m_writePos{0}
{
	m_capacity = m_size * BUFFER_COUNT;
	m_buffer.resize(m_capacity);
}

NetBridge::RecvBuffer::~RecvBuffer()
{
	m_buffer.clear();
}

bool NetBridge::RecvBuffer::OnWrite(const uint32 numOfBytes)
{
	if(numOfBytes > GetFreeSize())
		return false;

	m_writePos += numOfBytes;

	return true;
}

bool NetBridge::RecvBuffer::OnRead(const uint32 numOfBytes)
{
	if(numOfBytes > GetDataSize())
		return false;

	m_readPos += numOfBytes;

	return true;
}

void NetBridge::RecvBuffer::Clean()
{
	const int32 dataSize = GetDataSize();
	if(dataSize == 0) {
		m_readPos = m_writePos = 0;
	}
	else {
		if(GetFreeSize() < m_size) {
			memcpy_s(&m_buffer[0], m_capacity, &m_buffer[m_readPos], dataSize);
			m_readPos = 0;
			m_writePos = dataSize;
		}
	}
}