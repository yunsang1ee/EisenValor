#pragma once

#include "RIOBuffer.h"

namespace ServerEngine {
	class RIOSession;

	class RecvBuffer : public RIOBuffer {
	private:
		explicit RecvBuffer();
		virtual ~RecvBuffer();
		friend class RIOSession;
	};
}


