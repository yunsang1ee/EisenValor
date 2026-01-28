#pragma once

#include "RIOBuffer.h"

namespace ServerEngine {

	namespace RIO {
		class RIOSession;
		
		class RIORecvBuffer : public RIOBuffer {
		private:
			explicit RIORecvBuffer();
			virtual ~RIORecvBuffer();
			friend class RIO::RIOSession;
		};
	}
}