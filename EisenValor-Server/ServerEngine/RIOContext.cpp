#include "pch.h"
#include "RIOContext.h"

#ifdef _USE_RIO
GameServerEngine::RIO::RIOContext::RIOContext(IO_CONTEXT_TYPE type)
	:m_type{ type }, m_owner{ nullptr }
{
	Init();
}

void GameServerEngine::RIO::RIOContext::Init()
{
	RIO_BUF::BufferId = 0;
	RIO_BUF::Offset = 0;
	RIO_BUF::Length = 0;
}
#endif