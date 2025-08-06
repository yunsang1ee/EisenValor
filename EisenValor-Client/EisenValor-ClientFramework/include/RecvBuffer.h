#pragma once

namespace NetBridge
{
class NetworkManager;
class RecvBuffer
{
	enum
	{
		BUFFER_COUNT = 10
	};

private:
	std::vector<char> m_buffer;
	uint32			  m_size;
	uint32			  m_capacity;
	uint32			  m_readPos;
	uint32			  m_writePos;

public:
	explicit RecvBuffer(const uint32 bufferSize = NW_BUFFER_CAPACITY);
	~RecvBuffer();

private:
	RecvBuffer(const RecvBuffer&) = delete;
	RecvBuffer& operator=(const RecvBuffer&) = delete;
	RecvBuffer(RecvBuffer&&) noexcept = delete;
	RecvBuffer& operator=(RecvBuffer&&) noexcept = delete;

public:
	bool OnWrite(const uint32 numOfBytes);
	bool OnRead(const uint32 numOfBytes);
	void Clean();

	uint32		GetDataSize() const noexcept { return m_writePos - m_readPos; }
	uint32		GetFreeSize() const noexcept { return m_capacity - m_writePos; }
	char*		GetWritePos() noexcept { return &m_buffer[m_writePos]; }
	const char* GetReadPos() const noexcept { return &m_buffer[m_readPos]; }
};
} // namespace NetBridge
