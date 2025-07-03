#include "pch.h"
#include "RIOBuffer.h"

#include "RIOCore.h"

ServerEngine::RIOBuffer::RIOBuffer(const uint32 capacity)
	:m_id{RIO_INVALID_BUFFERID}, m_buffer{nullptr}, m_capacity{capacity}
{
	m_buffer = reinterpret_cast<char*>(VirtualAllocEx(GetCurrentProcess(), 0, m_capacity, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));

	if(nullptr == m_buffer)
		ServerEngine::LogManager::PrintLastError();

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	const uint64 granularity = systemInfo.dwAllocationGranularity;

	assert(m_capacity % granularity == 0);

	if(nullptr != m_buffer)
		m_id = RIO_EXT_FUNC_TB.RIORegisterBuffer(m_buffer, m_capacity);

	if(RIO_INVALID_BUFFERID == m_id)
		ServerEngine::LogManager::PrintLastError();
}

ServerEngine::RIOBuffer::~RIOBuffer()
{
	RIO_EXT_FUNC_TB.RIODeregisterBuffer(m_id);

	if(0 == VirtualFreeEx(GetCurrentProcess(), m_buffer, 0, MEM_RELEASE))
		ServerEngine::LogManager::PrintLastError();
}
