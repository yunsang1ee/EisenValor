#include "pch.h"
#include "IOContext.h"

LobbyServerEngine::IOContext::IOContext(const IO_CONTEXT_TYPE type)
	:m_type{ type }, m_owner{ nullptr }
{
	Init();
}

void LobbyServerEngine::IOContext::Init()
{
	OVERLAPPED::hEvent = 0;
	OVERLAPPED::Internal = 0;
	OVERLAPPED::InternalHigh = 0;
	OVERLAPPED::Offset = 0;
	OVERLAPPED::OffsetHigh = 0;
}