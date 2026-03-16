#include "pch.h"
#include "AcceptThread.h"

#include "WorkerThread.h"
#include "ServerEngineCore.h"
#include "Session.h"
#include "IOCoreTest.h"

#ifdef  MODERN_CODE
ServerEngine::AcceptThread::AcceptThread(const SessionFactoryFunc func, const DWORD listenSocketFlags, WorkerThread* const ownerWorker)
	: m_serverAddress{}, m_func{func}, m_ownerWorker{ownerWorker}
{
	m_listenSocket = CreateSocket(listenSocketFlags);
}

ServerEngine::AcceptThread::~AcceptThread()
{
	if(m_listenSocket != INVALID_SOCKET) {
		closesocket(m_listenSocket);
		m_listenSocket = INVALID_SOCKET;
	}
}

bool ServerEngine::AcceptThread::Init(const uint16 port)
{
	if(m_listenSocket == INVALID_SOCKET)
		return false;

	m_port = port;

	memset(&m_serverAddress, 0, sizeof(m_serverAddress));
	m_serverAddress.sin_family = AF_INET;
	m_serverAddress.sin_port = htons(port);
	m_serverAddress.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	constexpr int opt{ 1 };
	setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int));

	u_long mode = 1;
	ioctlsocket(m_listenSocket, FIONBIO, &mode);

	if(SOCKET_ERROR == bind(m_listenSocket, (SOCKADDR*)&m_serverAddress, sizeof(m_serverAddress))) {
		LOG_WSA_GET_LAST_ERROR();
		return false;
	}

	if(SOCKET_ERROR == listen(m_listenSocket, SOMAXCONN)) {
		LOG_WSA_GET_LAST_ERROR();
		return false;
	}

	return true;
}

void ServerEngine::AcceptThread::Run(const std::stop_token st)
{
	while(false == st.stop_requested()) {
		sockaddr_in clientAddr;
		int addrLen{ sizeof(clientAddr) };
	RETRY:
		SOCKET clientSocket{ accept(m_listenSocket, (SOCKADDR*)&clientAddr, &addrLen) };
		if(INVALID_SOCKET == clientSocket) {
			if(st.stop_requested())
				break;
			goto RETRY;
		}

		SetSocketOptions(clientSocket);

		std::wstring ipAddress;
		ipAddress.resize(100);
		InetNtopW(AF_INET, &clientAddr.sin_addr, ipAddress.data(), ipAddress.size());

		LOG_INFO("Session Connected! IP = {}, PORT = {}", WStringToString(ipAddress.c_str()), clientAddr.sin_port);

		auto session = m_func();

		if(false == session->AcceptCompleted(clientSocket, clientAddr)) {
			std::cout << "Failed AcceptCompleted" << std::endl;
			continue;
		}

		m_ownerWorker->PushJob(&ServerEngine::WorkerThread::Register, (session));
	}
}

void ServerEngine::AcceptThread::SetSocketOptions(SOCKET& socket)
{
	u_long arg{ 1 };
	ioctlsocket(socket, FIONBIO, &arg);

	// NAGLE Algorithm off
	int opt{ 1 };
	setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(int));
}
#endif //  MODERN_CODE
