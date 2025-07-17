#pragma once

#include "RIOBuffer.h"

namespace ServerEngine {
	class Session;

	class RecvBuffer : public RIOBuffer {
	private:
		explicit RecvBuffer(const uint32 bufferSize = MAX_RIO_BUFFER_SIZE);
		virtual ~RecvBuffer();
		friend class Session;
	};
}


