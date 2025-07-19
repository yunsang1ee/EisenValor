#pragma once

#include "RIOBuffer.h"

namespace ServerEngine {
	class SendBuffer : public RIOBuffer {
		enum { BUFFER_COUNT = 10, };
	private:
		uint32 m_sendOffset;

	public:
		explicit SendBuffer(const uint32 capacity = MAX_RIO_BUFFER_SIZE);
		~SendBuffer();

	public:
		bool			Append(const char* const data, const uint32 size) noexcept;
		uint32			GetDataSizeForCurrentPacket() const noexcept { return m_writePos - m_sendOffset; }
		bool			moveSendOffset(const uint32 bytesTransferred);
		uint32			GetSendOffset() const noexcept { return m_sendOffset; }
		virtual void	CleanBuffer() noexcept override;
	};
}