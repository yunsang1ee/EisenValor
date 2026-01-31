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
} // namespace Utils