#pragma once
#include "IRenderData.h"
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <utility>

template <typename T>
concept IsValidRenderData = std::derived_from<T, IRenderData> && requires {
	{ T::StaticRuntimeTypeID() } -> std::convertible_to<RenderDataTypeID>;
};

template <IsValidRenderData T, RenderDataPolicy PolicyValue>
class RenderDataStorageBase : public RenderDataBase<T>
{
public:
	using DataType = T;

	static constexpr RenderDataPolicy StaticPolicy() { return PolicyValue; }
};

template <typename T>
concept IsRenderDataStorage = std::derived_from<T, IRenderData> && requires {
	typename T::DataType;
	requires IsValidRenderData<typename T::DataType>;
	{ T::StaticRuntimeTypeID() } -> std::convertible_to<RenderDataTypeID>;
	{ T::StaticPolicy() } -> std::same_as<RenderDataPolicy>;
};

template <IsValidRenderData T, size_t Count>
class FrameBuffered : public RenderDataStorageBase<T, RenderDataPolicy::FrameBuffered>
{
public:
	static_assert(Count > 0, "FrameBuffered requires at least one slot.");
	using DataType = T;

	FrameBuffered() = default;

	void BeginFrame(uint32_t frameIndex) override
	{
		m_currentIndex = frameIndex % static_cast<uint32_t>(Count);
		m_slots[m_currentIndex].BeginFrame(frameIndex);
	}

	void EndFrame() override { m_slots[m_currentIndex].EndFrame(); }
	void OnResize(uint32_t width, uint32_t height) override
	{
		for (auto& slot : m_slots)
		{
			slot.OnResize(width, height);
		}
	}
	void Release() override
	{
		for (auto& slot : m_slots)
		{
			slot.Release();
		}
	}

	bool IsValid() const override { return m_slots[m_currentIndex].IsValid(); }

	DataType&		GetCurrent() { return m_slots[m_currentIndex]; }
	const DataType& GetCurrent() const { return m_slots[m_currentIndex]; }

	DataType&		Get(uint32_t frameIndex) { return m_slots[frameIndex % static_cast<uint32_t>(Count)]; }
	const DataType& Get(uint32_t frameIndex) const { return m_slots[frameIndex % static_cast<uint32_t>(Count)]; }

	DataType&		operator[](size_t index) { return m_slots[index]; }
	const DataType& operator[](size_t index) const { return m_slots[index]; }

	constexpr size_t Size() const { return Count; }
	uint32_t		 CurrentIndex() const { return m_currentIndex; }

private:
	std::array<DataType, Count> m_slots = {};
	uint32_t					m_currentIndex = 0;
};

enum class HistorySwapMode : uint8_t
{
	Manual,
	AutoEndFrame
};

template <IsValidRenderData T>
class PingPongHistory : public RenderDataStorageBase<T, RenderDataPolicy::PingPongHistory>
{
public:
	using DataType = T;

	PingPongHistory() = default;
	explicit PingPongHistory(HistorySwapMode swapMode) : m_swapMode(swapMode) {}

	void EndFrame() override
	{
		if (HistorySwapMode::AutoEndFrame == m_swapMode)
		{
			Swap();
		}
	}
	void OnResize(uint32_t width, uint32_t height) override
	{
		for (auto& slot : m_slots)
		{
			slot.OnResize(width, height);
		}
	}
	void ResetHistory() override
	{
		for (auto& slot : m_slots)
		{
			slot.ResetHistory();
		}
		Reset();
	}
	void Release() override
	{
		for (auto& slot : m_slots)
		{
			slot.Release();
		}
	}

	bool IsValid() const override { return m_slots[m_readIndex].IsValid(); }

	DataType&		Read() { return m_slots[m_readIndex]; }
	const DataType& Read() const { return m_slots[m_readIndex]; }

	DataType&		Write() { return m_slots[m_writeIndex]; }
	const DataType& Write() const { return m_slots[m_writeIndex]; }

	DataType&		Slot(uint32_t index) { return m_slots[index & 1u]; }
	const DataType& Slot(uint32_t index) const { return m_slots[index & 1u]; }

	void Swap()
	{
		std::swap(m_readIndex, m_writeIndex);
		++m_frameCount;
	}

	void Reset()
	{
		m_readIndex = 0;
		m_writeIndex = 1;
		m_frameCount = 0;
	}

	uint32_t		ReadIndex() const { return m_readIndex; }
	uint32_t		WriteIndex() const { return m_writeIndex; }
	uint32_t		FrameCount() const { return m_frameCount; }
	HistorySwapMode SwapMode() const { return m_swapMode; }
	void			SetSwapMode(HistorySwapMode swapMode) { m_swapMode = swapMode; }

private:
	std::array<DataType, 2> m_slots = {};
	uint32_t				m_readIndex = 0;
	uint32_t				m_writeIndex = 1;
	uint32_t				m_frameCount = 0;
	HistorySwapMode			m_swapMode = HistorySwapMode::Manual;
};

template <IsValidRenderData T>
class Persistent : public RenderDataStorageBase<T, RenderDataPolicy::Persistent>
{
public:
	using DataType = T;

	Persistent() = default;

	void BeginFrame(uint32_t frameIndex) override { m_value.BeginFrame(frameIndex); }
	void EndFrame() override { m_value.EndFrame(); }
	void OnResize(uint32_t width, uint32_t height) override { m_value.OnResize(width, height); }
	void ResetHistory() override { m_value.ResetHistory(); }
	void Release() override { m_value.Release(); }

	bool IsValid() const override { return m_value.IsValid(); }

	DataType&		Get() { return m_value; }
	const DataType& Get() const { return m_value; }

	void Set(const DataType& value)
	{
		m_value = value;
		MarkDirty();
	}

	void Set(DataType&& value)
	{
		m_value = std::move(value);
		MarkDirty();
	}

	bool IsDirty() const { return m_dirty; }
	void MarkDirty()
	{
		m_dirty = true;
		++m_version;
	}
	void	 ClearDirty() { m_dirty = false; }
	uint64_t Version() const { return m_version; }

private:
	T		 m_value = {};
	bool	 m_dirty = true;
	uint64_t m_version = 0;
};

template <IsValidRenderData T>
class Transient : public RenderDataStorageBase<T, RenderDataPolicy::Transient>
{
public:
	using DataType = T;

	Transient() = default;

	void BeginFrame(uint32_t frameIndex) override { m_value.BeginFrame(frameIndex); }
	void EndFrame() override { m_value.EndFrame(); }
	void OnResize(uint32_t width, uint32_t height) override { m_value.OnResize(width, height); }
	void ResetHistory() override { m_value.ResetHistory(); }
	void Release() override { m_value.Release(); }

	bool IsValid() const override { return m_value.IsValid(); }

	DataType& Get() { return m_value; }
	const T&  Get() const { return m_value; }

private:
	DataType m_value = {};
};
