#include "pch.h"
#include "RIOSendBuffer.h"

ServerEngine::RIO::RIOSendBuffer::RIOSendBuffer()
	:m_sendOffset{0}
{
}

ServerEngine::RIO::RIOSendBuffer::~RIOSendBuffer()
{
}

bool ServerEngine::RIO::RIOSendBuffer::Append(const char* const data, const uint32 size) noexcept
{
	if(size > GetFreeSize())
		return false;

	memcpy(GetWriteCursor(), data, size);

	if(false == OnWrite(size))
		return false;

	return true;
}

bool ServerEngine::RIO::RIOSendBuffer::moveSendOffset(const uint32 bytesTransferred)
{
	if(bytesTransferred > GetFreeSize())
		return false;

	m_sendOffset += bytesTransferred;

	return true;
}

void ServerEngine::RIO::RIOSendBuffer::CleanBuffer() noexcept
{
	const uint32 dataSize = GetDataSizeForCurrentPacket();

	if(dataSize == 0) {
		m_readPos = m_writePos = m_sendOffset = 0;
	}
	else {
		if(GetFreeSize() < m_size) {
			::memcpy(&m_buffer[0], &m_buffer[m_readPos], dataSize);
			m_readPos = 0;
			m_writePos = dataSize;
		}
	}
}
