#include "pch.h"
#include "RIORingBuffer.h"

GameServerEngine::RIORingBuffer::RIORingBuffer(const uint32 capacity)
	:m_bufferId(RIO_INVALID_BUFFERID), m_buffer{ nullptr }, m_capacity{ capacity }, m_usedSize{}, m_readOffset{}, m_writeOffset{}
{
	Alloc();
}

GameServerEngine::RIORingBuffer::~RIORingBuffer()
{
}

bool GameServerEngine::RIORingBuffer::RegisterBuffer(const RIO_EXTENSION_FUNCTION_TABLE& rioFuncTable)
{
	m_bufferId = rioFuncTable.RIORegisterBuffer(m_buffer, m_capacity);

	if(m_bufferId == RIO_INVALID_BUFFERID) {
		LOG_ERROR("RIORegisterBuffer Failed!");
		return false;
	}
	return true;
}

bool GameServerEngine::RIORingBuffer::OnWrite(const uint32 len)
{
    if(len > GetFreeSize())
        return false;

    m_writeOffset = (m_writeOffset + len) % m_capacity;
    m_usedSize += len;
    
    return true;
}

bool GameServerEngine::RIORingBuffer::OnRead(const uint32 len)
{
    if(len > m_usedSize) 
        return false;

    m_readOffset = (m_readOffset + len) % m_capacity;
    m_usedSize -= len;
    
    return true;
}

void GameServerEngine::RIORingBuffer::AdjustPos()
{
    if(m_usedSize == 0) {
        m_readOffset = m_writeOffset = 0;
        return;
    }

    if(m_writeOffset >= m_readOffset) {
        if(m_readOffset > 0) {
            std::cout << "AdjustPos: m_writeOffset >= m_readOffset && m_readOffset > 0" << std::endl;
            ::memmove(m_buffer, m_buffer + m_readOffset, m_usedSize);
            m_readOffset = 0;
            m_writeOffset = m_usedSize;
        }
    }
    else {
        std::cout << "AdjustPos: m_readOffset > m_writeOffset" << std::endl;
        char* temp = (char*)_malloca(m_usedSize);
        uint32 firstPart = m_capacity - m_readOffset;
        uint32 secondPart = m_writeOffset;

        ::memcpy(temp, m_buffer + m_readOffset, firstPart);
        ::memcpy(temp + firstPart, m_buffer, secondPart);
        ::memcpy(m_buffer, temp, m_usedSize);

        m_readOffset = 0;
        m_writeOffset = m_usedSize;
    }
}

uint32 GameServerEngine::RIORingBuffer::GetContiguousReadSize() const
{
    if(m_writeOffset >= m_readOffset)
        return m_writeOffset - m_readOffset;

    return m_capacity - m_readOffset;
}

uint32 GameServerEngine::RIORingBuffer::GetContiguousFreeSize() const
{
    if(m_writeOffset >= m_readOffset)
        return m_capacity - m_writeOffset;

    return m_readOffset - m_writeOffset;
}

void GameServerEngine::RIORingBuffer::CleanBuffer()
{
    if(GetUsedSize() == 0) {
        m_readOffset = m_writeOffset = 0;
    }
    else if(GetFreeSize() < (m_capacity / 4)) {
        AdjustPos();
    }
}

void GameServerEngine::RIORingBuffer::Alloc()
{
	m_buffer = reinterpret_cast<char*>(VirtualAllocEx(GetCurrentProcess(), 0, m_capacity, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));

	if(nullptr == m_buffer)
		LOG_ERROR("RIORingBuffer Alloc Failed!");
}

GameServerEngine::RIORingRecvBuffer::RIORingRecvBuffer()
{
}

GameServerEngine::RIORingRecvBuffer::~RIORingRecvBuffer()
{
}

GameServerEngine::RIORingSendBuffer::RIORingSendBuffer()
{
}

GameServerEngine::RIORingSendBuffer::~RIORingSendBuffer()
{
}

bool GameServerEngine::RIORingSendBuffer::Append(const char* const data, const uint32 size)
{
    if(GetFreeSize() < size) return false;

    uint32 firstPart = std::min(size, m_capacity - m_writeOffset);
    ::memcpy(m_buffer + m_writeOffset, data, firstPart);

    if(size > firstPart) {
        ::memcpy(m_buffer, data + firstPart, size - firstPart);
    }

    return true;
}

void GameServerEngine::RIORingSendBuffer::moveSendOffset(const uint32 size)
{
    m_writeOffset = (m_writeOffset + size) % m_capacity;
    m_usedSize += size;
}
