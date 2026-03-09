#include "pch.h"
#include "Session.h"

LobbyServerEngine::Session::Session()
	:m_socket{ INVALID_SOCKET }
{
}

LobbyServerEngine::Session::~Session()
{
	closesocket(m_socket);
}

void LobbyServerEngine::Session::Send(std::shared_ptr<PacketBuffer> pb)
{
	if(false == IsConnected())
		return;

	bool posted{ false };

	m_packetBufferQueue.push(std::move(pb));

	if(m_sendPosted.exchange(true) == false)
		posted = true;

	if(posted)
		PostSend();
}

void LobbyServerEngine::Session::Dispatch(IOContext* const context, const uint32 bytesTransferred)
{
	switch(context->GetType()) {
		case IO_CONTEXT_TYPE::RECV:
			ProcessRecv(bytesTransferred);
			break;
		case IO_CONTEXT_TYPE::SEND:
			ProcessSend(bytesTransferred);
			break;
		default:
			break;
	}
}

void LobbyServerEngine::Session::PostRecv()
{
	if(false == IsConnected())
		return;

	m_recvContext.Init();
	m_recvContext.SetOwner(std::static_pointer_cast<LobbyServerEngine::Session>(shared_from_this()));

	
}

void LobbyServerEngine::Session::ProcessRecv(const uint32 bytesTransferred)
{

}

void LobbyServerEngine::Session::PostSend()
{

}

void LobbyServerEngine::Session::ProcessSend(const uint32 bytesTransferred)
{
	if(false == IsConnected())
		return;

	m_recvContext.Init();
	m_recvContext.SetOwner(std::static_pointer_cast<LobbyServerEngine::Session>(shared_from_this()));


}
