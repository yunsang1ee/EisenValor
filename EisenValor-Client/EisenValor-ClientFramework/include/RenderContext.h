#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include "IRenderData.h"


class RenderContext
{
public:
	// ========================================
	// Transient 리소스 (프레임 내 Pass 간 공유)
	// ========================================
	template <typename T>
	void SetTransient(const std::string& name, T* resource)
	{
		static_assert(std::is_base_of_v<IRenderData, T>);
		m_transient[name] = resource;
	}

	template <typename T>
	T* GetTransient(const std::string& name) const
	{
		auto it = m_transient.find(name);
		return it != m_transient.end() ? static_cast<T*>(it->second) : nullptr;
	}

	// ========================================
	// Persistent 리소스 (이전 프레임 데이터)
	// ========================================
	template <typename T>
	void SetPersistent(const std::string& name, T* resource)
	{
		static_assert(std::is_base_of_v<IRenderData, T>);
		m_persistent[name] = resource;
	}

	template <typename T>
	T* GetPersistent(const std::string& name) const
	{
		auto it = m_persistent.find(name);
		return it != m_persistent.end() ? static_cast<T*>(it->second) : nullptr;
	}

	// ========================================
	// Temporal 리소스 (N-2, N-3, ... 프레임)
	// ========================================
	template <typename T>
	void SetTemporal(const std::string& name, uint32_t frameOffset, T* resource)
	{
		std::string key = name + "_F" + std::to_string(frameOffset);
		m_temporal[key] = resource;
	}

	template <typename T>
	T* GetTemporal(const std::string& name, uint32_t frameOffset) const
	{
		std::string key = name + "_F" + std::to_string(frameOffset);
		auto		it = m_temporal.find(key);
		return it != m_temporal.end() ? static_cast<T*>(it->second) : nullptr;
	}

	// ========================================
	// Lifecycle
	// ========================================
	void BeginFrame() { m_transient.clear(); }

	void SwapPersistent()
	{
		for (auto& [name, res] : m_persistent)
		{
			m_temporal[name + "_F1"] = res;
		}
		m_persistent.clear();
	}

private:
	std::unordered_map<std::string, IRenderData*> m_transient;	// 프레임 내
	std::unordered_map<std::string, IRenderData*> m_persistent; // N-1 프레임
	std::unordered_map<std::string, IRenderData*> m_temporal;	// N-2, N-3, ...
};