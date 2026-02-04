#include "pch.h"
#include "IOCPRecvBuffer.h"

#ifdef _USE_IOCP
ServerEngine::IOCP::IOCPRecvBuffer::IOCPRecvBuffer(const int32 bufferSize)
	:m_bufferSize{bufferSize}
{
	m_capacity = m_bufferSize * BUFFER_COUNT;
	m_buffer.resize(m_capacity);
}

ServerEngine::IOCP::IOCPRecvBuffer::~IOCPRecvBuffer()
{
}

void ServerEngine::IOCP::IOCPRecvBuffer::Clean()
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

bool ServerEngine::IOCP::IOCPRecvBuffer::OnRead(int32 numOfBytes)
{
	if(numOfBytes > DataSize())
		return false;

	m_readPos += numOfBytes;
	return true;
}

bool ServerEngine::IOCP::IOCPRecvBuffer::OnWrite(int32 numOfBytes)
{
	if(numOfBytes > FreeSize())
		return false;

	m_writePos += numOfBytes;
	return true;
}
#endif