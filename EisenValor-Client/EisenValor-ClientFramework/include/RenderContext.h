#pragma once
#include <vector>
#include <deque>
#include <cassert>
#include "IRenderData.h"

class RenderContext
{
public:
	RenderContext() = default;
	~RenderContext() = default;

	template <IsValidRenderData T>
	void SetData(T* data, uint32_t lifetime = 0)
	{
		if (nullptr == data)
		{
			return;
		}

		const uint32_t typeID = T::StaticRuntimeTypeID();

		if (typeID >= m_histories.size())
		{
			m_histories.resize(typeID + 1);
		}

		auto& history = m_histories[typeID];

		DataEntry entry;
		entry.data = data;
		entry.maxLifetime = lifetime;
		entry.currentAge = 0;

		history.push_front(entry);
	}

	template <IsValidRenderData T>
	T* GetData(uint32_t offset = 0) const
	{
		const uint32_t typeID = T::StaticRuntimeTypeID();

		if (typeID >= m_histories.size())
		{
			return nullptr;
		}

		const auto& history = m_histories[typeID];
		if (offset >= history.size())
		{
			return nullptr;
		}

		return static_cast<T*>(history[offset].data);
	}

	void UpdateLifetimes()
	{
		for (auto& history : m_histories)
		{
			if (history.empty())
			{
				continue;
			}

			for (auto it = history.begin(); it != history.end();)
			{
				if (it->currentAge >= it->maxLifetime)
				{
					it = history.erase(it);
				}
				else
				{
					it->currentAge++;
					++it;
				}
			}
		}
	}

private:
	struct DataEntry
	{
		IRenderData* data;
		uint32_t	 maxLifetime;
		uint32_t	 currentAge;
	};

	std::vector<std::deque<DataEntry>> m_histories;
};