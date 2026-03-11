#include "pch.h"
#include "IOCPCore.h"

#include "IOCPObject.h"
#include "IOContext.h"
#include "Session.h"

LobbyServerEngine::IOCPCore::IOCPCore()
{
	m_iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
}

LobbyServerEngine::IOCPCore::~IOCPCore()
{
	CloseHandle(m_iocpHandle);
}

bool LobbyServerEngine::IOCPCore::Register(std::shared_ptr<IOCPObject> iocpObject)
{
	return ::CreateIoCompletionPort(iocpObject->GetHandle(), m_iocpHandle, 0, 0);
}

void LobbyServerEngine::IOCPCore::Dispatch(const uint32 timeoutMS)
{
	DWORD		numOfBytes{};
	ULONG_PTR	key{};
	IOContext* ioContext{ nullptr };

	if(::GetQueuedCompletionStatus(m_iocpHandle, &numOfBytes, &key, reinterpret_cast<LPOVERLAPPED*>(&ioContext), timeoutMS)) {
		auto iocpObject = ioContext->GetOwner();
		iocpObject->Dispatch(ioContext, numOfBytes);
	}
	else {
		int32 errCode = ::WSAGetLastError();

		switch(errCode) {
			case WAIT_TIMEOUT:
				return;
			case ERROR_NETNAME_DELETED:
			{
				auto session = ioContext->GetOwner();
				session->Dispatch(ioContext, numOfBytes);
				break;
			}
			default:
			{
				break;
			}
		}
	}
}