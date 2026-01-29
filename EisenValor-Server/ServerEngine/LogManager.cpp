#include "pch.h"
#include "LogManager.h"

std::ostringstream ServerEngine::LogManager::s_oss;
std::mutex ServerEngine::LogManager::s_logMutex;

void ServerEngine::LogManager::Init() noexcept
{
#ifdef _USE_IOCP
	std::string filePath{ "LOG/IOCP" };
#endif

#ifdef _USE_RIO
	std::string filePath{ "LOG/RIO" };
#endif

	if(false == std::filesystem::exists(filePath))
		std::filesystem::create_directory(filePath);
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