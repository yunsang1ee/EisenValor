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
	void SetData(std::shared_ptr<T> data, uint32_t lifetime = 0)
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
		entry.data = std::static_pointer_cast<IRenderData>(data);
		entry.maxLifetime = lifetime;
		entry.currentAge = 0;

		history.push_front(std::move(entry));
	}

	template <IsValidRenderData T>
	std::shared_ptr<T> GetData(uint32_t offset = 0) const
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

		return std::static_pointer_cast<T>(history[offset].data);
	}

	void UpdateLifetimes()
	{
		for (auto& history : m_histories)
		{
			if (history.empty())
			{
				continue;
			}

			auto it = history.begin();
			while (it != history.end())
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
		std::shared_ptr<IRenderData> data;
		uint32_t					 maxLifetime;
		uint32_t					 currentAge;
	};

	std::vector<std::deque<DataEntry>> m_histories;
};