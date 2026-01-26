#include "pch.h"
#include "RIOBuffer.h"

#include "RIOCore.h"
#include "NetworkManager.h"

ServerEngine::RIOBuffer::RIOBuffer()
	:m_id{ RIO_INVALID_BUFFERID }, m_buffer{ nullptr }, m_readPos{ 0 }, m_writePos{ 0 }
{
}

ServerEngine::RIOBuffer::~RIOBuffer()
{
	if(nullptr != m_buffer) {
		RIO_EXT_FUNC_TB.RIODeregisterBuffer(m_id);

		if(0 == VirtualFreeEx(GetCurrentProcess(), m_buffer, 0, MEM_RELEASE))
			ServerEngine::LogManager::PrintLastError();
	}
}

void ServerEngine::RIOBuffer::Init(const uint32 bufferSize)
{
	m_size = bufferSize;
	m_capacity = bufferSize * BUFFER_COUNT;

	m_buffer = reinterpret_cast<char*>(VirtualAllocEx(GetCurrentProcess(), 0, m_capacity, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));

	if(nullptr == m_buffer)
		ServerEngine::LogManager::PrintLastError();

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo); // 기본 페이지 4kb
	const uint64 granularity = systemInfo.dwAllocationGranularity;	// 메모리 할당할 때 이 숫자의 배수로 메모리 반환해줌. (64kb 65536)

	assert(m_capacity % granularity == 0);

	if(nullptr != m_buffer)
		m_id = RIO_EXT_FUNC_TB.RIORegisterBuffer(m_buffer, m_capacity);

	if(RIO_INVALID_BUFFERID == m_id)
		ServerEngine::LogManager::PrintLastError();
}

bool ServerEngine::RIOBuffer::OnRead(const uint32 numOfBytes)
{
	if(numOfBytes > GetDataSize())
		return false;

	m_readPos += numOfBytes;

	return true;
}

bool ServerEngine::RIOBuffer::OnWrite(const uint32 numOfBytes)
{
	if(numOfBytes > GetFreeSize())
		return false;

	m_writePos += numOfBytes;

	return true;
}

void ServerEngine::RIOBuffer::CleanBuffer() noexcept
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