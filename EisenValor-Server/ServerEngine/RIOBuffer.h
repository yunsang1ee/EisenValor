#pragma once

namespace ServerEngine {
	class RIOBuffer {
	protected:
		RIO_BUFFERID	m_id;
		char*			m_buffer;
		uint32			m_capacity;

	public:
		RIOBuffer(const uint32 capacity);
		virtual ~RIOBuffer();
	};
}


