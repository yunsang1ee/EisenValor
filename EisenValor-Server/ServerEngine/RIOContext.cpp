#include "pch.h"
#include "RIOContext.h"

ServerEngine::RIOContext::RIOContext(RIO_CONTEXT_TYPE type)
	:m_type{ type }, m_session{ nullptr }
{
	Init();
}

void ServerEngine::RIOContext::Init()
{
	RIO_BUF::BufferId = 0;
	RIO_BUF::Offset = 0;
	RIO_BUF::Length = 0;
}

ServerEngine::RecvContext::RecvContext()
	:RIOContext(RIO_CONTEXT_TYPE::RECV)
{
}

ServerEngine::SendContext::SendContext()
	:RIOContext(RIO_CONTEXT_TYPE::SEND)
{
}
