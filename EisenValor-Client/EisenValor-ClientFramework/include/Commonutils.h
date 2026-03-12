#pragma once
#include <cstdint>
#include <string>
#include <chrono>
#include <filesystem>

#pragma region Utils
namespace Utils
{
// FNV-1a Hash https://share.google/trFAqACv1zHhll7h8
constexpr uint64_t HashString(std::string_view str)
{
	uint64_t hash = 14695981039346656037ULL;
	for (char c : str)
	{
		hash ^= static_cast<uint8_t>(c);
		hash *= 1099511628211ULL;
	}
	return hash;
}

std::string GetTimestamp();

const std::filesystem::path& ExeDir();

inline std::string WideToUtf8(const wchar_t* wstr)
{
	if (!wstr || wstr[0] == L'\0')
	{
		return {};
	}

	const int lenWithNull = ::WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
	if (lenWithNull <= 0)
	{
		return {};
	}

	std::string buffer(static_cast<size_t>(lenWithNull), '\0');

	const int written = ::WideCharToMultiByte(CP_UTF8, 0, wstr, -1, buffer.data(), lenWithNull, nullptr, nullptr);
	if (written != lenWithNull)
	{
		return {};
	}

	buffer.pop_back();
	return buffer;
}
} // namespace Utils
#pragma endregion

#pragma region DebugHelpers

#ifdef _DEBUG
#include <fstream>
#include <iostream>
#include <format>
#include <mutex>
#include <windows.h>
namespace Utils
{
inline void LogToFile(const std::string& message)
{
	static std::mutex	 logMutex;
	static std::ofstream logFile;

	std::lock_guard<std::mutex> lock(logMutex);

	if (!logFile.is_open())
	{
		std::filesystem::path logDirPath = Utils::ExeDir() / "Logs";
		std::filesystem::create_directories(logDirPath);

		std::filesystem::path logFilePath = logDirPath / std::format("EisenValor_Log_{}.txt", GetCurrentProcessId());

		const bool isNewFile = !std::filesystem::exists(logFilePath);
		logFile.open(logFilePath, std::ios::out | std::ios::app | std::ios::binary);

		if (isNewFile && logFile.is_open())
		{
			constexpr char utf8Bom[] = "\xEF\xBB\xBF";
			logFile.write(utf8Bom, 3);
		}
	}

	logFile.write(message.data(), static_cast<std::streamsize>(message.size()));
	logFile.flush();
}

template <typename... Args>
inline void DebugLogFmt(std::format_string<Args...> fmt, Args&&... args)
{
	std::string msg = GetTimestamp() + std::format(fmt, std::forward<Args>(args)...);
	std::cout << msg;
	LogToFile(msg);
}
} // namespace Utils
#define DEBUG_LOG_FMT(...) Utils::DebugLogFmt(__VA_ARGS__)
#else
#define DEBUG_LOG_FMT(...) ((void)0)
#endif

#pragma endregion