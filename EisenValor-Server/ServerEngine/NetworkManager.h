#pragma once

#include "Singleton.hpp"

namespace ServerEngine {
	class IOCore;
#ifdef LEGACY_CODE
	class NetworkManager : public Singleton<NetworkManager> {
		SINGLETON(NetworkManager)
	public:
		bool Init(const SessionFactoryFunc func);
		void Run();
		void Shutdown();

	public:
		IOCore* GetIOCore() const { return m_ioCore.get(); }

	private:
		std::unique_ptr<IOCore> m_ioCore;
	};
#endif
}


