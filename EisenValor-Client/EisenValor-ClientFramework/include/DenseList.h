#pragma once
#include <vector>
#include <concepts>
#include <cassert>
#include <cstdint>
#include <span>
#include <utility>

template <typename T>
	requires std::movable<T>
class DenseList
{
public:
	struct Handle
	{
		uint32_t id = 0;
		uint32_t generation = 0;

		static Handle Invalid() { return Handle{0, 0}; }

		bool	 operator==(const Handle& other) const = default;
		explicit operator bool() const noexcept { return generation != 0; }

		uint64_t GetValue() const { return (static_cast<uint64_t>(generation) << 32) | id; }
	};

	using value_type = T;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using reference = T&;
	using const_reference = const T&;
	using pointer = T*;
	using const_pointer = const T*;

	using iterator = typename std::vector<T>::iterator;
	using const_iterator = typename std::vector<T>::const_iterator;
	using reverse_iterator = typename std::vector<T>::reverse_iterator;
	using const_reverse_iterator = typename std::vector<T>::const_reverse_iterator;

public:
	DenseList() = default;
	~DenseList() = default;

	DenseList(const DenseList&) = delete;
	DenseList& operator=(const DenseList&) = delete;

	DenseList(DenseList&&) noexcept = default;
	DenseList& operator=(DenseList&&) noexcept = default;

public:
	[[nodiscard]] bool IsValid(Handle handle) const noexcept
	{
		return handle.generation != 0 && handle.id < m_generation.size() &&
			   m_generation[handle.id] == handle.generation;
	}

	template <typename... Args>
	[[nodiscard]] Handle Emplace(Args&&... args)
	{
		uint32_t id;
		if (!m_freeIds.empty())
		{
			id = m_freeIds.back();
			m_freeIds.pop_back();
		}
		else
		{
			id = static_cast<uint32_t>(m_generation.size());
			m_generation.push_back(1);
			m_idToIndex.push_back(0);
			m_indexToId.push_back(0);
		}

		uint32_t index = static_cast<uint32_t>(m_data.size());
		m_data.emplace_back(std::forward<Args>(args)...);

		m_idToIndex[id] = index;
		if (index >= m_indexToId.size())
		{
			m_indexToId.push_back(id);
		}
		else
		{
			m_indexToId[index] = id;
		}
		return Handle{id, m_generation[id]};
	}

	template <typename... Args>
	[[nodiscard]] Handle Add(Args&&... args)
	{
		return Emplace(std::forward<Args>(args)...);
	}

	void Remove(Handle handle)
	{
		if (!IsValid(handle))
		{
			return;
		}
		uint32_t index = m_idToIndex[handle.id];
		uint32_t lastIndex = static_cast<uint32_t>(m_data.size() - 1);
		if (index != lastIndex)
		{
			m_data[index] = std::move(m_data[lastIndex]);
			uint32_t lastId = m_indexToId[lastIndex];
			m_idToIndex[lastId] = index;
			m_indexToId[index] = lastId;
		}
		m_data.pop_back();
		m_indexToId.pop_back();

		m_generation[handle.id]++;
		m_freeIds.push_back(handle.id);
	}

	[[nodiscard]] pointer TryGet(Handle handle) noexcept
	{
		if (!IsValid(handle))
		{
			return nullptr;
		}
		return &m_data[m_idToIndex[handle.id]];
	}

	[[nodiscard]] const_pointer TryGet(Handle handle) const noexcept
	{
		if (!IsValid(handle))
		{
			return nullptr;
		}
		return &m_data[m_idToIndex[handle.id]];
	}

	[[nodiscard]] reference Get(Handle handle)
	{
		assert(IsValid(handle) && "[DenseList] Get - Accessing invalid handle");
		return m_data[m_idToIndex[handle.id]];
	}

	[[nodiscard]] const_reference Get(Handle handle) const
	{
		assert(IsValid(handle) && "[DenseList] Get - Accessing invalid handle");
		return m_data[m_idToIndex[handle.id]];
	}

	reference		operator[](Handle handle) { return Get(handle); }
	const_reference operator[](Handle handle) const { return Get(handle); }

public:
	iterator	   begin() noexcept { return m_data.begin(); }
	iterator	   end() noexcept { return m_data.end(); }
	const_iterator begin() const noexcept { return m_data.begin(); }
	const_iterator end() const noexcept { return m_data.end(); }
	const_iterator cbegin() const noexcept { return m_data.cbegin(); }
	const_iterator cend() const noexcept { return m_data.cend(); }

	reverse_iterator	   rbegin() noexcept { return m_data.rbegin(); }
	reverse_iterator	   rend() noexcept { return m_data.rend(); }
	const_reverse_iterator rbegin() const noexcept { return m_data.rbegin(); }
	const_reverse_iterator rend() const noexcept { return m_data.rend(); }
	const_reverse_iterator crbegin() const noexcept { return m_data.crbegin(); }
	const_reverse_iterator crend() const noexcept { return m_data.crend(); }

	const_pointer data() const noexcept { return m_data.data(); }
	size_type	  size() const noexcept { return m_data.size(); }
	bool		  empty() const noexcept { return m_data.empty(); }

	void Reserve(size_t cap)
	{
		m_data.reserve(cap);
		m_idToIndex.reserve(cap);
		m_indexToId.reserve(cap);
		m_generation.reserve(cap);
		m_freeIds.reserve(cap);
	}
	void Clear()
	{
		m_data.clear();
		m_indexToId.clear();
		m_idToIndex.clear();
		m_generation.clear();
		m_freeIds.clear();
	}
	std::span<const T> GetSpan() const { return {m_data.data(), m_data.size()}; }

private:
	std::vector<T> m_data;

	std::vector<uint32_t> m_indexToId;
	std::vector<uint32_t> m_idToIndex;
	std::vector<uint32_t> m_generation;
	std::vector<uint32_t> m_freeIds;
};