#pragma once

namespace ServerEngine {
	class LogManager {
	public:
		enum class LOG_LEVEL {
			INFO,
			WARNING,
			ERR,
			TRACE,

			END
		};

	private:
		LogManager() = delete;
		~LogManager() = delete;

		LogManager(const LogManager&) = delete;
		LogManager(LogManager&&) = delete;

		LogManager& operator=(const LogManager&) = delete;
		LogManager& operator=(LogManager&&) = delete;

	public:
		static void Init() noexcept;

		template<typename... Args>
		static void PrintLog(const LOG_LEVEL level, const Args... msg) noexcept
		{
			const auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
			const auto localTime = std::chrono::zoned_time(std::chrono::current_zone(), now);

			static const std::array<std::string, static_cast<uint8>(LOG_LEVEL::END)> arrLevel{ "[INFO]", "[WARNING]", "[ERROR]", "[TRACE]" };

			std::ostringstream oss;
			oss << std::format("{:%Y-%m-%d %H:%M:%S} {}: ", localTime, arrLevel[static_cast<int>(level)]);
			((oss << msg << " "), ...);
			oss << "\n";

			const HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

			CONSOLE_SCREEN_BUFFER_INFO csbi;
			GetConsoleScreenBufferInfo(consoleHandle, &csbi);
			WORD oldColor = csbi.wAttributes;

			static const std::array<BYTE, 4> arrLevelColor { FOREGROUND_GREEN, FOREGROUND_GREEN | FOREGROUND_RED, FOREGROUND_RED,FOREGROUND_INTENSITY };

			SetConsoleTextAttribute(consoleHandle, arrLevelColor[static_cast<int>(level)]);
			std::cout << std::format("{}", oss.str());
			SetConsoleTextAttribute(consoleHandle, oldColor);

			Save(oss);
		}

		static void PrintLastError(const std::source_location& loc = std::source_location::current()) noexcept;

	private:
		static void Save(const std::ostringstream& oss)
		{
			const auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
			const auto localTime = std::chrono::zoned_time(std::chrono::current_zone(), now);
#ifdef _DEBUG
			const std::string fileName = std::format("LOG/[DEBUG] {:%Y-%m-%d %H%M}.txt", localTime).c_str();
#else
			const std::string fileName = std::format("LOG/[RELEASE] {:%Y-%m-%d %H%M}.txt", localTime).c_str();
#endif // _DEBUG

			std::ofstream ofs{ fileName,  std::ios::out | std::ios::app };
			ofs << oss.str();
		}
	};


}


