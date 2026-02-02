#pragma once

#include "RIOBuffer.h"

namespace ServerEngine {

	namespace RIO {
		class RIOSession;
		
#ifdef _USE_RIO
		class RIORecvBuffer : public RIOBuffer {
		private:
			explicit RIORecvBuffer();
			virtual ~RIORecvBuffer();
			friend class RIO::RIOSession;
		};
#endif
	}
}