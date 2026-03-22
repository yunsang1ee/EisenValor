#pragma once

namespace GameServerEngine {
	class LogManager {
	public:
		enum class LOG_LEVEL : uint8 {
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
		static void Init();


		template<typename... Args>
		static void WriteLog(const LOG_LEVEL level, const std::format_string<Args...> fmtStr, Args&&... args)
		{
			using LogLevel = std::pair<std::string_view, WORD>;

			static std::osyncstream oss{ std::cout };

			const auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
			const auto localTime = std::chrono::zoned_time(std::chrono::current_zone(), now);

			static const std::array<LogLevel, static_cast<uint8>(LOG_LEVEL::END)> arrLevelToColor = { {
				{ "[INFO]", static_cast<WORD>(FOREGROUND_GREEN)},
				{ "[WARNING]", static_cast<WORD>(FOREGROUND_GREEN | FOREGROUND_RED) },
				{ "[ERROR]", static_cast<WORD>(FOREGROUND_RED) },
				{ "[TRACE]", static_cast<WORD>(FOREGROUND_INTENSITY) }
			} };

			std::stringstream ss;
			ss << std::format("{:%Y-%m-%d %H:%M:%S} {} ", localTime, arrLevelToColor[static_cast<int>(level)].first.data());
			ss << std::format(fmtStr, std::forward<Args>(args)...);
			ss << "\n";
			{
				std::lock_guard<std::mutex> lk{ s_logMutex };
				const HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

				CONSOLE_SCREEN_BUFFER_INFO csbi;
				GetConsoleScreenBufferInfo(consoleHandle, &csbi);
				WORD oldColor = csbi.wAttributes;

				SetConsoleTextAttribute(consoleHandle, arrLevelToColor[static_cast<int>(level)].second);
				std::cout << ss.str();
				SetConsoleTextAttribute(consoleHandle, oldColor);

				s_oss << ss.str();
			}
		}

		static void PrintLastError(const std::source_location& loc = std::source_location::current());

	public:
		static void Save();

	public:
		static std::mutex s_logMutex;
		static std::ostringstream s_oss;

	};


}


