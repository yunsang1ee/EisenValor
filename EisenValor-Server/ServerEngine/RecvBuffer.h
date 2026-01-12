#pragma once

#include "RIOBuffer.h"

namespace ServerEngine {
	class Session;

	class RecvBuffer : public RIOBuffer {
	private:
		explicit RecvBuffer();
		virtual ~RecvBuffer();
		friend class Session;
	};
}


