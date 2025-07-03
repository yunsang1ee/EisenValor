#pragma once

namespace Server {
	class ServerManager {
	private:
		ServerManager() = delete;
		~ServerManager() = delete;
		
		ServerManager(const ServerManager&) = delete;
		ServerManager& operator=(const ServerManager&) = delete;
		
		ServerManager(ServerManager&&) noexcept = delete;
		ServerManager& operator=(ServerManager&&) noexcept = delete;

	public:
		static void Init() noexcept;
		static void Run() noexcept;
		static void Shutdown() noexcept;
	};
}


