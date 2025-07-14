#pragma once

#include "RIOBuffer.h"
namespace ServerEngine {
	class RecvBuffer : public RIOBuffer {
	private:
		uint32			m_count;
		uint32			m_size;
		uint32			m_readPos;
		uint32			m_writePos;

	public:
		explicit RecvBuffer(const uint32 capacity = MAX_RIO_BUFFER_CAPACITY);
		virtual ~RecvBuffer();

	public:
		bool			OnRead(const uint32 numOfBytes);
		bool			OnWrite(const uint32 numOfBytes);
		void			Clean();

	public:
		RIO_BUFFERID	GetID() const noexcept { return m_id; }
		char*			GetBuffer() noexcept { return m_buffer; }
		char*			GetWritePos() noexcept { return &m_buffer[m_writePos]; }
		const char*		GetReadPos() const noexcept { return &m_buffer[m_readPos]; }

		ULONG			GetWriteOffset() const noexcept { return static_cast<ULONG>(m_buffer - &m_buffer[m_writePos]); }
		ULONG			GetReadOffset() const noexcept { return static_cast<ULONG>(m_buffer- &m_buffer[m_readPos]); }

	public:
		const uint32	GetDataSize() { return m_writePos - m_readPos; }
		const uint32	GetFreeSize() { return m_capacity - m_writePos; }
	
	};
}


