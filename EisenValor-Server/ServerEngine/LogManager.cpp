#include "pch.h"
#include "LogManager.h"

std::ostringstream ServerEngine::LogManager::s_oss;
std::mutex ServerEngine::LogManager::s_logMutex;

void ServerEngine::LogManager::Init() noexcept
{
	std::string filePath{ "LOG/" };
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

void ServerEngine::LogManager::Save()
{
	const auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
	const auto localTime = std::chrono::zoned_time(std::chrono::current_zone(), now);

#ifdef _DEBUG
	const std::string fileName = std::format("LOG/[DEBUG] {:%Y-%m-%d %H%M} KST.txt", localTime).c_str();
#else
	const std::string fileName = std::format("LOG/[RELEASE] {:%Y-%m-%d %H%M} KST.txt", localTime).c_str();
#endif // _DEBUG

	std::ofstream ofs{ fileName,  std::ios::out | std::ios::app };
	ofs << s_oss.str();
}
