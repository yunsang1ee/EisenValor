#pragma once

namespace LobbyServerEngine {
	class IOCPObject : public std::enable_shared_from_this<IOCPObject> {
	public:
		virtual HANDLE GetHandle() const abstract;
		virtual void Dispatch(class IOContext* const ioContext, const uint32 bytesTransferred) abstract;
	};
}