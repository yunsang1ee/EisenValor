#pragma once

namespace GameServerEngine {
	class RIORingBuffer {
	protected:
		RIO_BUFFERID	m_bufferId;  
		char*			m_buffer;
		uint32			m_capacity;
		uint32			m_usedSize;
		uint32			m_readOffset;      
		uint32			m_writeOffset;      
	
	public:
		explicit RIORingBuffer(const uint32 capacity = 655360);
		virtual ~RIORingBuffer();

	public:
		bool RegisterBuffer(const RIO_EXTENSION_FUNCTION_TABLE& rioFuncTable);
		void DeregisterBuffer(const RIO_EXTENSION_FUNCTION_TABLE& rioFuncTable);
		uint32 GetFreeSize() const { return m_capacity - m_usedSize; }
		uint32 GetUsedSize() const { return m_usedSize; }
		
		uint32 GetWriteOffset() const { return m_writeOffset; }

		RIO_BUF GetWriteRioBuf() const { return { m_bufferId, (ULONG)m_writeOffset, (ULONG)GetContiguousFreeSize() }; }

		bool OnWrite(const uint32 len);

		bool OnRead(const uint32 len);

		void AdjustPos();

		char* GetReadPos() { return m_buffer + m_readOffset; }
		
		uint32 GetContiguousReadSize() const;
		uint32 GetContiguousFreeSize() const;

		char* GetReadCursor() { return GetReadPos(); }

		void CleanBuffer();

		RIO_BUFFERID GetID() const { return m_bufferId; }
		uint32 GetCapacity() const { return m_capacity; }

	private:
		void Alloc();
		void Free();
	};


	// ==========================================
	// 				RIORingRecvBuffer
	// ==========================================
	class RIORingRecvBuffer : public RIORingBuffer {
	public:
		RIORingRecvBuffer();
		virtual ~RIORingRecvBuffer();
	};

	// ==========================================
	// 				RIORingSendBuffer
	// ==========================================
	class RIORingSendBuffer : public RIORingBuffer {
	public:
		RIORingSendBuffer();
		virtual ~RIORingSendBuffer();

	public:
		bool Append(const char* const data, const uint32 size);

		void moveSendOffset(const uint32 size);

		uint32 GetSendOffset() const { return m_writeOffset; }

		uint32 GetDataSizeForCurrentPacket(uint32 lastAppendSize) const { return lastAppendSize; }
	};
}


