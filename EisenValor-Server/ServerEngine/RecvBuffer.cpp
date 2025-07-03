#include "pch.h"
#include "RecvBuffer.h"

#include "RIOCore.h"

ServerEngine::RecvBuffer::RecvBuffer(const uint32 capacity)
	:RIOBuffer(capacity), m_size{MAX_RIO_BUFFER_SIZE}, m_count{MAX_RIO_BUFFER_COUNT}, m_readPos{0}, m_writePos{0}
{
}

ServerEngine::RecvBuffer::~RecvBuffer()
{

}

bool ServerEngine::RecvBuffer::OnRead(const uint32 numOfBytes)
{
	if(numOfBytes > GetDataSize())
		return false;

	m_readPos += numOfBytes;

	return true;
}

bool ServerEngine::RecvBuffer::OnWrite(const uint32 numOfBytes)
{
	if(numOfBytes > GetFreeSize())
		return false;

	m_writePos += numOfBytes;

	return true;
}

void ServerEngine::RecvBuffer::Clean()
{
	const uint32 dataSize = GetDataSize();

	if(dataSize == 0) {
		m_readPos = m_writePos = 0;
	}
	else {
		if(GetFreeSize() < m_size) {
			::memcpy(&m_buffer[0], &m_buffer[m_readPos], dataSize);
			m_readPos = 0;
			m_writePos = dataSize;
		}
	}
}
