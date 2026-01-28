#include "pch.h"
#include "IOCPCore.h"

#include "ServerEngineConfigManager.h"

#include "Session.h"

ServerEngine::IOCP::IOCPCore::IOCPCore()
	:m_iocpHandle{ INVALID_HANDLE_VALUE }, m_acceptContext{}
{

}

bool ServerEngine::IOCP::IOCPCore::Init(const SessionFactoryFunc func)
{
	if(false == IOCore::Init(func)) return false;

	m_iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	if(INVALID_HANDLE_VALUE == m_iocpHandle) return false;

	m_listenSocket = CreateSocket(WSA_FLAG_OVERLAPPED);

	if(INVALID_SOCKET == m_listenSocket) return false;
	if(false == RegistHandle(m_listenSocket)) return false;

	constexpr int opt{ 1 };
	setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int));

	if(SOCKET_ERROR == bind(m_listenSocket, (SOCKADDR*)&m_serverAddress, sizeof(m_serverAddress))) {
		LOG_WSA_GET_LAST_ERROR();
		return false;
	}

	m_workerThreadCount = MANAGER(ServerEngine::ThreadManager)->GetWorkerThreadCount();

	return true;
}

bool ServerEngine::IOCP::IOCPCore::StartAccept()
{
	if(false == IOCore::StartAccept()) return false;

	RegistAccept();

	return true;
}

void ServerEngine::IOCP::IOCPCore::Run()
{
	for(uint16 i = 0; i < m_workerThreadCount; ++i) {
		MANAGER(ServerEngine::ThreadManager)->EnqueueTask([this, i](const std::stop_token& st)
			{
				TLS_THREAD_ID = i + 1;
				while(false == st.stop_requested()) {
					TLS_WORK_END_TIME = high_resolution_clock::now() + TLS_ALLOCATED_WORK_TIME;
					Dispatch(10);
					DistributeReservedTask();
					FlushTaskQueue();
				}
			});
	}
}

void ServerEngine::IOCP::IOCPCore::Shutdown()
{
	::CloseHandle(m_iocpHandle);
	shutdown(m_listenSocket, SD_BOTH);
	closesocket(m_listenSocket);
}

bool ServerEngine::IOCP::IOCPCore::RegistHandle(const SOCKET socket)
{
	if(INVALID_HANDLE_VALUE == CreateIoCompletionPort(reinterpret_cast<HANDLE>(socket), m_iocpHandle, 0, 0))
		return false;

	return true;
}

void ServerEngine::IOCP::IOCPCore::Dispatch(const uint32 timeoutMs)
{
	DWORD			numOfBytes{};
	ULONG_PTR		key{};
	IOCPContext* iocpContext{ nullptr };

	if(::GetQueuedCompletionStatus(m_iocpHandle, &numOfBytes, &key, reinterpret_cast<LPOVERLAPPED*>(&iocpContext), timeoutMs)) {
		if(nullptr == iocpContext->GetOwner() && IO_CONTEXT_TYPE::ACCEPT == iocpContext->GetType()) {
			const IOCPAcceptContext* const acceptContext{ static_cast<IOCPAcceptContext*>(iocpContext) };
			ProcessAccept(acceptContext);
		}
		else {
			auto session = iocpContext->GetOwner();
			session->Dispatch(iocpContext, numOfBytes);
		}
	}
	else {
		int32 errCode = ::WSAGetLastError();

		switch(errCode) {
			case WAIT_TIMEOUT:
				return;
			case ERROR_NETNAME_DELETED:
			{
				auto session = iocpContext->GetOwner();
				session->Dispatch(iocpContext, numOfBytes);
				break;
			}
			default:
			{
				break;
			}
		}
	}
}

void ServerEngine::IOCP::IOCPCore::ProcessAccept(const IOCPAcceptContext* const acceptContext)
{
	const SOCKET acceptSocket{ acceptContext->GetAcceptSocket() };

	int32 optName{ SO_UPDATE_ACCEPT_CONTEXT };
	::setsockopt(acceptSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, reinterpret_cast<char*>(&m_listenSocket), sizeof(SOCKET));

	auto session{ CreateSession() };

	SOCKADDR_IN clientaddr;
	int32 addrlen{ sizeof(clientaddr) };
	if(SOCKET_ERROR == GetPeerName(acceptSocket, reinterpret_cast<SOCKADDR*>(&clientaddr), &addrlen)) {
		RegistAccept();
		return;
	}

	std::cout << "Client Accept Success!" << std::endl;

	std::wstring ipAddress;
	ipAddress.resize(100);
	InetNtopW(AF_INET, &clientaddr.sin_addr, ipAddress.data(), ipAddress.size());

	LOG_INFO("Session Connected! IP = {}, PORT = {}", WStringToString(ipAddress.c_str()), clientaddr.sin_port);

	session->AcceptCompleted(acceptSocket, clientaddr);

	RegistAccept();
}

bool ServerEngine::IOCP::IOCPCore::BindWindowsFunction(SOCKET socket, GUID guid, LPVOID* fn)
{
	DWORD bytes{};
	return SOCKET_ERROR != ::WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), fn, sizeof(*fn), OUT & bytes, NULL, NULL);
}

void ServerEngine::IOCP::IOCPCore::Close(SOCKET& socket)
{
	if(socket != INVALID_SOCKET)
		::closesocket(socket);
	socket = INVALID_SOCKET;
}

void ServerEngine::IOCP::IOCPCore::RegistAccept()
{
	const SOCKET acceptSocket{ CreateSocket(WSA_FLAG_OVERLAPPED) };

	if(false == RegistHandle(acceptSocket)) return;

	m_acceptContext.Init();
	m_acceptContext.SetAcceptSocket(acceptSocket);

	DWORD bytesReceived{};

	if(false == AcceptEx(m_listenSocket, acceptSocket, m_acceptContext.GetBuff(), 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &bytesReceived, static_cast<LPOVERLAPPED>(&m_acceptContext))) {
		const int32 errCode = ::WSAGetLastError();
		if(errCode != WSA_IO_PENDING) {
			RegistAccept();
		}
	}
}

std::shared_ptr<ServerEngine::IOCP::IOCPSession> ServerEngine::IOCP::IOCPCore::CreateSession()
{
#ifdef  IOCP_SERVER

	auto session = m_sessionFactoryFunc();

	return session;
#endif //  IOCP_SERVER

	return nullptr;
}
