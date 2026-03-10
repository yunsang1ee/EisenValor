#include "pch.h"
#include "RecvBuffer.h"

LobbyServerEngine::RecvBuffer::RecvBuffer(const int32 bufferSize)
	:m_bufferSize{ bufferSize }, m_readPos{}, m_writePos{}
{
	m_capacity = m_bufferSize * BUFFER_COUNT;
	m_buffer.resize(m_capacity);
}

LobbyServerEngine::RecvBuffer ::~RecvBuffer()
{
}

void LobbyServerEngine::RecvBuffer::Clean()
{
	int32 dataSize = DataSize();
	if(dataSize == 0) {
		m_readPos = m_writePos = 0;
	}
	else {
		if(FreeSize() < m_bufferSize) {
			::memcpy(&m_buffer[0], &m_buffer[m_readPos], dataSize);
			m_readPos = 0;
			m_writePos = dataSize;
		}
	}
}

bool LobbyServerEngine::RecvBuffer::OnRead(int32 numOfBytes)
{
	if(numOfBytes > DataSize())
		return false;

	m_readPos += numOfBytes;
	return true;
}

bool LobbyServerEngine::RecvBuffer::OnWrite(int32 numOfBytes)
{
	if(numOfBytes > FreeSize())
		return false;

	m_writePos += numOfBytes;
	return true;
}