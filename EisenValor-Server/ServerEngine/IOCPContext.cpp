#include "pch.h"
#include "IOCPContext.h"

#ifdef _USE_IOCP
ServerEngine::IOCP::IOCPContext::IOCPContext(const IO_CONTEXT_TYPE type)
	:m_type{type}, m_owner{nullptr}
{
	Init();
}

void ServerEngine::IOCP::IOCPContext::Init()
{
	OVERLAPPED::hEvent = 0;
	OVERLAPPED::Internal = 0;
	OVERLAPPED::InternalHigh = 0;
	OVERLAPPED::Offset = 0;
	OVERLAPPED::OffsetHigh = 0;
}
#endif
