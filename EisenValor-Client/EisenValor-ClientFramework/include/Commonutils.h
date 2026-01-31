#pragma once
#include <cstdint>
#include <string>

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
} // namespace Utils
#pragma endregion

#pragma region DebugHelpers

#ifdef _DEBUG
#define DEBUG_LOG_FMT(fmt, ...)                                                                                        \
	do                                                                                                                 \
	{                                                                                                                  \
		std::cout << Utils::GetTimestamp();                                                                            \
		std::cout << std::format((fmt), __VA_ARGS__);                                                                  \
	} while (false)
#else
#define DEBUG_LOG_FMT(fmt, ...) ((void)0)
#endif

#pragma endregion