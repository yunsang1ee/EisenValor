#include "pch.h"
#include "IOCPCore.h"

#include "ServerEngineConfigManager.h"

#include "Session.h"

LPFN_CONNECTEX			ServerEngine::IOCP::IOCPCore::ConnectEx;
LPFN_DISCONNECTEX		ServerEngine::IOCP::IOCPCore::DisconnectEx;
LPFN_ACCEPTEX			ServerEngine::IOCP::IOCPCore::AcceptEx;

ServerEngine::IOCP::IOCPCore::IOCPCore()
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

	SOCKET dummySocket{ CreateSocket(WSA_FLAG_OVERLAPPED) };
	assert(BindWindowsFunction(dummySocket, WSAID_CONNECTEX, reinterpret_cast<LPVOID*>(&ConnectEx)));
	assert(BindWindowsFunction(dummySocket, WSAID_DISCONNECTEX, reinterpret_cast<LPVOID*>(&DisconnectEx)));
	assert(BindWindowsFunction(dummySocket, WSAID_ACCEPTEX, reinterpret_cast<LPVOID*>(&AcceptEx)));
	Close(dummySocket);

	constexpr int opt{ 1 };
	setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int));

	const uint16 PORT_NUM{ MANAGER(ServerEngineConfigManager)->GetNetworkConfig().port };
	memset(&m_serverAddress, 0, sizeof(m_serverAddress));
	m_serverAddress.sin_family = AF_INET;
	m_serverAddress.sin_port = htons(PORT_NUM);
	m_serverAddress.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

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
		MANAGER(ServerEngine::ThreadManager)->EnqueueTask([this](const std::stop_token& st)
			{
				while(false == st.stop_requested()) {
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
	IOCPContext*	iocpContext{ nullptr };

	if(::GetQueuedCompletionStatus(m_iocpHandle, OUT & numOfBytes, OUT & key, reinterpret_cast<LPOVERLAPPED*>(&iocpContext), timeoutMs)) {
		if(nullptr == iocpContext->GetOwner() && IO_CONTEXT_TYPE::ACCEPT == iocpContext->GetType()) {
			IOCPAcceptContext* acceptContext{ static_cast<IOCPAcceptContext*>(iocpContext) };
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
			}
				break;
			default:
			{
				break;
			}
		}
	}
}

void ServerEngine::IOCP::IOCPCore::ProcessAccept(IOCPAcceptContext* acceptContext)
{
	SOCKET acceptSocket = acceptContext->GetAcceptSocket();

	int32 optName{ SO_UPDATE_ACCEPT_CONTEXT };
	::setsockopt(acceptSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, reinterpret_cast<char*>(&m_listenSocket), sizeof(SOCKET));
	
	auto session = CreateSession();
	
	SOCKADDR_IN clientaddr;
	int32 addrlen{ sizeof(clientaddr) };
	getpeername(acceptSocket, reinterpret_cast<SOCKADDR*>(&clientaddr), &addrlen);

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
	const SOCKET acceptSocket{CreateSocket(WSA_FLAG_OVERLAPPED)};

	if(false == RegistHandle(acceptSocket)) return;

	m_acceptContext.Init();
	m_acceptContext.SetAcceptSocket(acceptSocket);

	DWORD bytesReceived{};
	char buff[1024];
	if(false == AcceptEx(m_listenSocket, acceptSocket, buff, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &bytesReceived, static_cast<LPOVERLAPPED>(&m_acceptContext))) {
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
