#pragma once
#include <vector>
#include <concepts>
#include <cassert>
#include <cstdint>
#include <span>
#include <limits>
#include <algorithm>

// DenseList
// - 장기 식별자는 Handle만 사용.
// - pointer/reference/iterator/span은 컨테이너 변경(Emplace/Remove/Clear/Reserve 등)
//	 이후 무효화될 수 있으므로 프레임/루프를 넘어 저장 금지.
// - 순회 중 임시 사용은 허용.

template <typename T>
struct DenseListHandle
{
	uint32_t id = 0;
	uint32_t generation = 0;

	DenseListHandle() = default;
	DenseListHandle(uint32_t _id, uint32_t _gen) : id(_id), generation(_gen) {}

	static DenseListHandle Invalid() { return DenseListHandle{0, 0}; }

	bool IsValid() const { return generation != 0; }

	static DenseListHandle FromValue(uint64_t value) noexcept
	{
		return DenseListHandle{static_cast<uint32_t>(value & 0xFFFFFFFF), static_cast<uint32_t>(value >> 32)};
	}

	uint64_t GetValue() const noexcept { return (static_cast<uint64_t>(generation) << 32) | id; }

	auto operator<=>(const DenseListHandle&) const = default;
};

template <typename T>
	requires std::movable<T> && std::destructible<T>
class DenseList
{
public:
	using Handle = DenseListHandle<T>;

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

private:
	static constexpr uint32_t kInvalidIndex = std::numeric_limits<uint32_t>::max();

	static void BumpGeneration(uint32_t& gen) noexcept
	{
		++gen;
		if (gen == 0)
			gen = 1;
	}

public:
	DenseList() : m_baseGeneration(1) {}
	~DenseList() = default;

	DenseList(const DenseList&) = delete;
	DenseList& operator=(const DenseList&) = delete;

	DenseList(DenseList&&) noexcept = default;
	DenseList& operator=(DenseList&&) noexcept = default;

public:
	[[nodiscard]] bool IsValid(Handle handle) const noexcept
	{
		// clang-format off
		return handle.generation != 0 && 
			handle.id < m_generation.size() && 
			handle.id < m_idToIndex.size() &&
			m_generation[handle.id] == handle.generation&&
			m_idToIndex[handle.id] != kInvalidIndex;
		// clang-format on
	}

	template <typename... Args>
		requires std::constructible_from<T, Args...>
	[[nodiscard]] Handle Emplace(Args&&... args)
	{
		const bool reuse = !m_freeIds.empty();
		uint32_t   id{};

		if (reuse)
		{
			id = m_freeIds.back();
		}
		else
		{
			assert(m_generation.size() < static_cast<size_type>(std::numeric_limits<uint32_t>::max()));
			id = static_cast<uint32_t>(m_generation.size());
		}

		struct Guard
		{
			DenseList* self = nullptr;
			bool	   genAdded = false;
			bool	   mapAdded = false;
			bool	   dataAdded = false;
			bool	   indexAdded = false;
			bool	   committed = false;

			~Guard()
			{
				if (committed)
					return;
				if (indexAdded)
					self->m_indexToId.pop_back();
				if (dataAdded)
					self->m_data.pop_back();
				if (mapAdded)
					self->m_idToIndex.pop_back();
				if (genAdded)
					self->m_generation.pop_back();
			}
		} g{this};

		if (!reuse)
		{
			m_generation.push_back(m_baseGeneration);
			g.genAdded = true;
			m_idToIndex.push_back(kInvalidIndex);
			g.mapAdded = true;
		}

		const uint32_t index = static_cast<uint32_t>(m_data.size());
		m_data.emplace_back(std::forward<Args>(args)...);
		g.dataAdded = true;

		m_indexToId.push_back(id);
		g.indexAdded = true;

		m_idToIndex[id] = index;
		if (reuse)
		{
			m_freeIds.pop_back();
		}

		g.committed = true;
		return Handle{id, m_generation[id]};
	}

	template <typename... Args>
		requires std::constructible_from<T, Args...>
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
		const uint32_t id = handle.id;
		const uint32_t index = m_idToIndex[id];
		const uint32_t lastIndex = static_cast<uint32_t>(m_data.size() - 1);
		const uint32_t lastId = m_indexToId[lastIndex];

		if (index != lastIndex)
		{
			m_data[index] = std::move(m_data[lastIndex]);

			m_idToIndex[lastId] = index;
			m_indexToId[index] = lastId;
		}
		m_data.pop_back();
		m_indexToId.pop_back();

		m_idToIndex[id] = kInvalidIndex;
		BumpGeneration(m_generation[handle.id]);
		m_freeIds.push_back(handle.id);
	}

	[[nodiscard]] Handle ReserveHandle()
	{
		uint32_t   id{};

		if (!m_freeIds.empty())
		{
			id = m_freeIds.back();
			m_freeIds.pop_back();
		}
		else
		{
			assert(m_generation.size() < static_cast<size_type>(std::numeric_limits<uint32_t>::max()));
			id = static_cast<uint32_t>(m_generation.size());
			m_generation.push_back(m_baseGeneration);
			m_idToIndex.push_back(kInvalidIndex);
		}

		return Handle{id, m_generation[id]};
	}

	template <typename... Args>
		requires std::constructible_from<T, Args...>
	void FulfillReservation(Handle handle, Args&&... args)
	{
		if (!IsValid(handle))
		{
			DEBUG_LOG_FMT(
				"[DenseList] ERROR: Invalid reservation handle (id={}, gen={})\n", handle.id, handle.generation
			);
			assert(false && "[DenseList] FulfillReservation - Invalid handle passed");
			return;
		}

		if (m_idToIndex[handle.id] != kInvalidIndex)
		{
			DEBUG_LOG_FMT(
				"[DenseList] ERROR: Reservation handle already fulfilled (id={}, gen={})\n", handle.id, handle.generation
			);
			assert(false && "[DenseList] FulfillReservation - Handle already fulfilled");
			return;
		}

		const uint32_t index = static_cast<uint32_t>(m_data.size());
		m_data.emplace_back(std::forward<Args>(args)...);
		m_indexToId.push_back(handle.id);
		m_idToIndex[handle.id] = index;
	}

	[[nodiscard]] bool IsReserved(Handle handle) const noexcept
	{
		return handle.generation != 0 && handle.id < m_generation.size() &&
			   m_generation[handle.id] == handle.generation &&
			   m_idToIndex[handle.id] == kInvalidIndex;
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
		m_freeIds.clear();

		const size_t genSize = m_generation.size();
		m_freeIds.reserve(genSize);
		for (uint32_t id = 0; id < static_cast<uint32_t>(genSize); ++id)
		{
			m_idToIndex[id] = kInvalidIndex;
			BumpGeneration(m_generation[id]);
			m_freeIds.push_back(id);
		}
	}

	void Reset()
	{
		uint32_t maxGen = m_baseGeneration;
		if (!m_generation.empty())
		{
			uint32_t currentMax = *std::max_element(m_generation.begin(), m_generation.end());
			if (currentMax > maxGen)
			{
				maxGen = currentMax;
			}
		}
		m_baseGeneration = maxGen;
		BumpGeneration(m_baseGeneration);

		std::vector<T>().swap(m_data);
		std::vector<uint32_t>().swap(m_indexToId);
		std::vector<uint32_t>().swap(m_idToIndex);
		std::vector<uint32_t>().swap(m_generation);
		std::vector<uint32_t>().swap(m_freeIds);
	}

	std::span<const T> GetSpan() const { return {m_data.data(), m_data.size()}; }

private:
	std::vector<T> m_data;

	uint32_t			  m_baseGeneration = 1;
	std::vector<uint32_t> m_indexToId;
	std::vector<uint32_t> m_idToIndex;
	std::vector<uint32_t> m_generation;
	std::vector<uint32_t> m_freeIds;
};