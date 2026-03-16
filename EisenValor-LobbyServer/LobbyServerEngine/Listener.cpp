#include "pch.h"
#include "Listener.h"

#include "LobbyServerEngineCore.h"
#include "IOContext.h"
#include "Session.h"

LobbyServerEngine::Listener::Listener()
	:m_listenSocket{ INVALID_SOCKET }, m_serverAddress{}
{
}

LobbyServerEngine::Listener::~Listener()
{
	closesocket(m_listenSocket);
}

void LobbyServerEngine::Listener::Dispatch(IOContext* const ioContext, const uint32 bytesTransferred)
{
	AcceptContext* acceptContext{ static_cast<AcceptContext*>(ioContext) };
	ProcessAccept(acceptContext);
}

bool LobbyServerEngine::Listener::StartAccept(const uint16 port)
{
	m_listenSocket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	if(INVALID_SOCKET == m_listenSocket)
		return false;

	if(false == MANAGER(LobbyServerEngineCore)->GetIocpCore().Register(shared_from_this()))
		return false;

	constexpr int opt{ 1 };
	setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int));

	memset(&m_serverAddress, 0, sizeof(m_serverAddress));
	m_serverAddress.sin_family = AF_INET;
	m_serverAddress.sin_port = htons(port);
	m_serverAddress.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	if(SOCKET_ERROR == bind(m_listenSocket, (SOCKADDR*)&m_serverAddress, sizeof(m_serverAddress))) {
		LOG_WSA_GET_LAST_ERROR;
		return false;
	}

	if(SOCKET_ERROR == listen(m_listenSocket, SOMAXCONN)) {
		LOG_WSA_GET_LAST_ERROR;
		return false;
	}

	const uint32 maxSessionCount{ 100 };

	for(uint32 i = 0; i < maxSessionCount; ++i) {
		std::unique_ptr<AcceptContext> acceptContext{ std::make_unique<AcceptContext>() };
		acceptContext->SetOwner(shared_from_this());
		m_acceptContexts.emplace_back(std::move(acceptContext));
		PostAccept(m_acceptContexts.back().get());
	}

	return true;
}

void LobbyServerEngine::Listener::PostAccept(AcceptContext* const acceptContext)
{
	std::shared_ptr<Session> session = MANAGER(LobbyServerEngineCore)->CreateClientSession();

	acceptContext->Init();
	acceptContext->SetSession(session);

	DWORD bytesReceived{};
	
	if(false == AcceptEx(m_listenSocket, session->GetSocket(), session->GetRecvBuffer().WritePos(), 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, OUT & bytesReceived, static_cast<LPOVERLAPPED>(acceptContext))) {
		const int32 errCode = ::WSAGetLastError();
		if(errCode != WSA_IO_PENDING) {
			PostAccept(acceptContext);
		}
	}
}

void LobbyServerEngine::Listener::ProcessAccept(AcceptContext* const acceptContext)
{
	std::shared_ptr<Session> session = acceptContext->GetSession();
	static uint32 idGen{ 1 };
	session->SetID(idGen++);

	SOCKADDR_IN sockAddress;
	int32 sizeOfSockAddr = sizeof(sockAddress);

	if(false == SocketUtils::SetUpdateAcceptSocket(session->GetSocket(), m_listenSocket)) {
		// 실패했더라도 Register 해줘야함.
		PostAccept(acceptContext);
		return;
	}

	if(SOCKET_ERROR == ::getpeername(session->GetSocket(), OUT reinterpret_cast<SOCKADDR*>(&sockAddress), &sizeOfSockAddr)) {
		PostAccept(acceptContext);
		return;
	}

	session->SetNetAddress(sockAddress);
	session->ProcessConnect();

	PostAccept(acceptContext);
}
