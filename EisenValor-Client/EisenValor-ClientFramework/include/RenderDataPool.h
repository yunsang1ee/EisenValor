#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include "IRenderData.h"

class IRenderDataPool
{
public:
	virtual ~IRenderDataPool() = default;

	virtual void BeginFrame() = 0;
	virtual void SwapPersistent() = 0;
};

class TestData : public RenderDataBase<TestData>
{
	int value = 0;
	std::string info;
};

template <IsValidRenderData T>
class RenderDataPool : public IRenderDataPool
{
public:
	void SetTransient(const std::string& name, T* data) { m_transient[name] = data; }

	T* GetTransient(const std::string& name)
	{
		auto it = m_transient.find(name);
		if (it != m_transient.end())
			return it->second;
		return nullptr;
	}

	void SetPersistent(const std::string& name, T* data) { m_current[name] = data; }

	T* GetPersistent(const std::string& name)
	{
		auto it = m_history.find(name);
		if (it != m_history.end())
			return it->second;
		return nullptr;
	}

	void SetTemporal(const std::string& name, uint32_t offset, T* data)
	{
		std::string key = name + "_" + std::to_string(offset);
		m_temporal[key] = data;
	}

	T* GetTemporal(const std::string& name, uint32_t offset)
	{
		std::string key = name + "_" + std::to_string(offset);
		auto		it = m_temporal.find(key);
		if (it != m_temporal.end())
			return it->second;
		return nullptr;
	}


	void BeginFrame() override
	{
		m_transient.clear();
		m_current.clear();
	}

	void SwapPersistent() override { std::swap(m_current, m_history); }

private:
	std::unordered_map<std::string, T*> m_transient;
	std::unordered_map<std::string, T*> m_current;
	std::unordered_map<std::string, T*> m_history;
	std::unordered_map<std::string, T*> m_temporal;
};

#if 0
using RenderDataKey = uint64_t;

template <IsValidRenderData T>
class VectorMap
{
private:
	static constexpr size_t kMaxNameLen = 64;

	struct Entry
	{
		RenderDataKey key;
		char		  name[kMaxNameLen];
		T*			  value;
	};

public:
	void InsertOrUpdate(std::string_view name)
	{
		RenderDataKey key = std::hash<std::string_view>{}(name);
		assert(!name.empty() && name.length() < kMaxNameLen);
		for (auto& entry : m_data)
		{
			if (entry.key == key && strcmp(entry.name, nameStr.c_str()) == 0)
			{
				entry.value = value;
				return;
			}
		}

		Entry newEntry;
		newEntry.key = key;
		newEntry.value = value;

		strcpy_s(newEntry.name, nameStr.c_str());
		m_data.push_back(newEntry);
	}

	T* Find(std::string_view nameStr) const
	{
		RenderDataKey key = std::hash<std::string>{}(nameStr);
		const char*	  targetName = nameStr.data();

		for (const auto& entry : m_data)
		{
			if (entry.key == key && strcmp(entry.name, targetName) == 0)
			{
				return entry.value;
			}
		}
		return nullptr;
	}

	void Clear() { m_data.clear(); }

	void Swap(VectorMap<T>& other) { m_data.swap(other.m_data); }

	void Reserve(size_t capacity) { m_data.reserve(capacity); }

private:
	std::vector<Entry> m_data;
};

#endif