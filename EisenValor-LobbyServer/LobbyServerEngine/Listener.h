#pragma once

#include "IOCPObject.h"
#include "IOContext.h"

namespace LobbyServerEngine {
	class AcceptContext;

	class Listener final : public IOCPObject {
	public:
		Listener();
		~Listener();

	public:
		virtual HANDLE GetHandle() const override final { return reinterpret_cast<HANDLE>(m_listenSocket); }
		virtual void Dispatch(class IOContext* const ioContext, const uint32 bytesTransferred) override final;

	public:
		bool StartAccept(const uint16 port);

	private:
		void PostAccept(AcceptContext* const acceptContext);
		void ProcessAccept(AcceptContext* const acceptContext);

	private:
		SOCKET										m_listenSocket;
		SOCKADDR_IN									m_serverAddress;
		std::vector<std::unique_ptr<AcceptContext>> m_acceptContexts;
	};
}


