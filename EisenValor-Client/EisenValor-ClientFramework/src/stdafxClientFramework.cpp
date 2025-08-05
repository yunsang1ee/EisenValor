// pch.cpp: 미리 컴파일된 헤더에 해당하는 소스 파일

#include "stdafxClientFramework.h"

// 미리 컴파일된 헤더를 사용하는 경우 컴파일이 성공하려면 이 소스 파일이 필요합니다.
#include <chrono>

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
