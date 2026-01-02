#include "pch.h"
#include "LogManager.h"

std::ostringstream ServerEngine::LogManager::s_oss;

void ServerEngine::LogManager::Init() noexcept
{
#ifdef _DEBUG
	if(false == std::filesystem::exists("../Debug/LOG"))
		std::filesystem::create_directory("../Debug/LOG");
#else
	if(false == std::filesystem::exists("LOG"))
		std::filesystem::create_directory("LOG");
#endif // DEBUG
}

void ServerEngine::LogManager::PrintLastError(const std::source_location& loc) noexcept
{
	const fs::path file_path = loc.file_name();

	std::cout << std::format("{}:{} | ", file_path.filename().string(), loc.line());

	const int32 errCode = WSAGetLastError();	
	WCHAR* lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, errCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);

	std::wcout << lpMsgBuf;
}