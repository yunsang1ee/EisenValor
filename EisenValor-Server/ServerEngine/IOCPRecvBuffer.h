#pragma once

namespace ServerEngine {
	namespace IOCP {
		class IOCPRecvBuffer {
			enum { BUFFER_COUNT = 10 };
		private:
			int32				m_capacity;
			int32				m_bufferSize;
			int32				m_readPos;
			int32				m_writePos;
			std::vector<char>	m_buffer;

		public:
			IOCPRecvBuffer(const int32 bufferSize);
			~IOCPRecvBuffer();

		public:
			void	Clean();
			bool	OnRead(int32 numOfBytes);
			bool	OnWrite(int32 numOfBytes);

			char* ReadPos() { return &m_buffer[m_readPos]; }
			char* WritePos() { return &m_buffer[m_writePos]; }
			int32	DataSize() { return m_writePos - m_readPos; }
			int32	FreeSize() { return m_capacity - m_writePos; }

		};
	}
}