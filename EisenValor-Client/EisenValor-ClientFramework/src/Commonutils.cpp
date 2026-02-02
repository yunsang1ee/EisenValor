#include "stdafxClientFramework.h"
#include "CommonUtils.h"

namespace Utils
{
std::string GetTimestamp()
{
	using namespace std::chrono;

	auto		now = system_clock::now();
	std::time_t tt = system_clock::to_time_t(now);

	std::tm tm_local;
	localtime_s(&tm_local, &tt);

	auto millisec = duration_cast<milliseconds>(now.time_since_epoch()) % seconds(1);

	char buffer[64];
	std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &tm_local);

	return std::format("[{}.{:03}] ", buffer, millisec.count());
}

const std::filesystem::path& ExeDir()
{
	static const std::filesystem::path s_exeDir = []() -> std::filesystem::path
	{
		std::wstring buffer;
		buffer.resize(MAX_PATH);

		while (true)
		{
			const DWORD capacity = static_cast<DWORD>(buffer.size());
			const DWORD len = ::GetModuleFileNameW(nullptr, buffer.data(), capacity);

			if (len == 0)
			{
				const DWORD err = ::GetLastError();
				ThrowWin32Error(err, "GetModuleFileNameW");
			}

			if (len < capacity)
			{
				buffer.resize(len);
				return std::filesystem::path(std::wstring_view(buffer.data(), len)).parent_path();
			}

			if (UNICODE_STRING_MAX_CHARS <= buffer.size())
			{
				ThrowWin32Error(ERROR_FILENAME_EXCED_RANGE, "Exe path too long or truncated");
			}

			size_t nextSize = std::min(buffer.size() * 2, static_cast<size_t>(UNICODE_STRING_MAX_CHARS));
			if (nextSize <= buffer.size())
			{
				ThrowWin32Error(ERROR_FILENAME_EXCED_RANGE, "Buffer size overflow while expanding");
			}

			buffer.resize(nextSize);
		}
	}();

	return s_exeDir;
}

} // namespace Utils