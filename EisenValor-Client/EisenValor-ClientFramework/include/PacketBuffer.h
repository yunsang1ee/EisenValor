#pragma once

namespace NetBridge
{
class PacketBuffer
{
private:
	std::vector<char> m_buffer;
	uint64			  m_capacity;

public:
	explicit PacketBuffer(const uint64 capacity);
	~PacketBuffer();

public:
	char*  GetBuffer() noexcept { return m_buffer.data(); }
	uint64 GetCapacity() const noexcept { return m_capacity; }

};
} // namespace NetBridge