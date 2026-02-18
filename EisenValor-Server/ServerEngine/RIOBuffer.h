#pragma once

namespace ServerEngine {
	namespace RIO {
#ifdef _USE_RIO
		class RIOBuffer {
			enum { BUFFER_COUNT = 10 };
		protected:
			RIO_BUFFERID	m_id;
			char* m_buffer;

			uint32			m_size;
			uint32			m_capacity;

			uint32			m_readPos;
			uint32			m_writePos;

		public:
			explicit RIOBuffer();
			virtual ~RIOBuffer();

		public:
			void Init(const uint32 bufferSize);

		public:
			bool				OnRead(const uint32 numOfBytes);
			bool				OnWrite(const uint32 numOfBytes);
			virtual void		CleanBuffer();

		public:
			RIO_BUFFERID		GetID() const { return m_id; }
			char* GetBuffer() { return m_buffer; }

			char* GetWriteCursor() { return &m_buffer[m_writePos]; }
			const char* GetReadCursor() const { return &m_buffer[m_readPos]; }

			uint32				GetWriteOffset() const { return m_writePos; }
			uint32				GetReadOffset() const { return m_readPos; }

			const uint32		GetDataSize() const { return m_writePos - m_readPos; }
			const uint32		GetFreeSize() const { return m_capacity - m_writePos; }

		};
#endif
	}
}


