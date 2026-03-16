#pragma once

namespace LobbyServerEngine {
	class IOCPObject;

	class IOCPCore {
	public:
		IOCPCore();
		~IOCPCore();

		IOCPCore(const IOCPCore&) = delete;
		IOCPCore& operator=(const IOCPCore&) = delete;
		IOCPCore(IOCPCore&&) noexcept = delete;
		IOCPCore& operator=(IOCPCore&&) noexcept = delete;

	public:
		bool Register(std::shared_ptr<IOCPObject> iocpObject);
		void Dispatch(const uint32 timeoutMS);
	
	public:
		HANDLE GetHandle() const { return m_iocpHandle; }

	private:
		HANDLE m_iocpHandle;
	};
}

