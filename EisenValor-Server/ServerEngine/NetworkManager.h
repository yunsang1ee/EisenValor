#pragma once

#include "Singleton.hpp"

namespace ServerEngine {
	class IOCore;
	class NetworkManager : public Singleton<NetworkManager> {
		SINGLETON(NetworkManager)
	private:
		IO_MODEL_TYPE			m_ioModelType;
		std::unique_ptr<IOCore> m_ioCore;

	public:
		bool Init(const IO_MODEL_TYPE ioModelType, const SessionFactoryFunc func);
		void Run();
		void Shutdown();

	public:
		IOCore* GetIOCore() const noexcept { return m_ioCore.get(); }
	};
}


