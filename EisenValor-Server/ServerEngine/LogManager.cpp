#include "pch.h"
#include "LogManager.h"

void ServerEngine::LogManager::Init() noexcept
{
	const auto path = std::filesystem::current_path();

	if(false == std::filesystem::exists("LOG"))
		std::filesystem::create_directory("LOG");
}

void ServerEngine::LogManager::PrintLastError() noexcept
{
	const int32 errCode = WSAGetLastError();

	WCHAR* lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, errCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);

	std::wcout << lpMsgBuf << std::endl;
}
