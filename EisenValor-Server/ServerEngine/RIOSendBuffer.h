#pragma once

#include "RIOBuffer.h"

namespace ServerEngine {
	class PacketBuffer;

	namespace RIO {
#ifdef _USE_RIO
		class RIOSendBuffer : public RIOBuffer {
		private:
			uint32 m_sendOffset;

		public:
			explicit RIOSendBuffer();
			~RIOSendBuffer();

		public:
			bool			Append(const char* const data, const uint32 size) noexcept;
			bool			moveSendOffset(const uint32 bytesTransferred);
			virtual void	CleanBuffer() noexcept override;

		public:
			uint32			GetDataSizeForCurrentPacket() const noexcept { return m_writePos - m_sendOffset; }
			uint32			GetSendOffset() const noexcept { return m_sendOffset; }
		};
#endif
	}
}