#pragma once
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>
#include "RenderDataPolicy.h"

enum class RenderDataAccessMode : uint8_t
{
	Read,
	Write,
	ReadWrite
};

struct RenderDataAccess
{
	std::string			 passName;
	RenderDataTypeID	 typeID = 0;
	RenderDataPolicy	 policy = RenderDataPolicy::FrameBuffered;
	RenderDataAccessMode mode = RenderDataAccessMode::Read;
	const char*			 typeName = "";
	const char*			 debugName = "";
};

template <typename T>
struct RenderDataSourceTraits;

template <IsValidRenderData T>
struct RenderDataSourceTraits<Transient<T>>
{
	using DataType = T;

	static constexpr RenderDataPolicy Policy = RenderDataPolicy::Transient;
	static RenderDataTypeID			  TypeID() { return T::StaticRuntimeTypeID(); }
	static IRenderData*				  Current(Transient<T>& data) { return &data.Get(); }
};

template <IsValidRenderData T, size_t Count>
struct RenderDataSourceTraits<FrameBuffered<T, Count>>
{
	using DataType = T;

	static constexpr RenderDataPolicy Policy = RenderDataPolicy::FrameBuffered;
	static RenderDataTypeID			  TypeID() { return T::StaticRuntimeTypeID(); }
	static IRenderData*				  Current(FrameBuffered<T, Count>& data) { return &data.GetCurrent(); }
};

template <IsValidRenderData T>
struct RenderDataSourceTraits<PingPongHistory<T>>
{
	using DataType = T;

	static constexpr RenderDataPolicy Policy = RenderDataPolicy::PingPongHistory;
	static RenderDataTypeID			  TypeID() { return T::StaticRuntimeTypeID(); }
	static IRenderData*				  Current(PingPongHistory<T>& data) { return &data.Write(); }
};

template <IsValidRenderData T>
struct RenderDataSourceTraits<Persistent<T>>
{
	using DataType = T;

	static constexpr RenderDataPolicy Policy = RenderDataPolicy::Persistent;
	static RenderDataTypeID			  TypeID() { return T::StaticRuntimeTypeID(); }
	static IRenderData*				  Current(Persistent<T>& data) { return &data.Get(); }
};

template <typename T>
concept IsRenderDataSource = requires(T& data) {
	typename RenderDataSourceTraits<T>::DataType;
	requires IsValidRenderData<typename RenderDataSourceTraits<T>::DataType>;
	{ RenderDataSourceTraits<T>::TypeID() } -> std::convertible_to<RenderDataTypeID>;
	{ RenderDataSourceTraits<T>::Current(data) } -> std::convertible_to<IRenderData*>;
};

class RenderContext
{
public:
	RenderContext() = default;
	~RenderContext() = default;

	template <IsRenderDataSource T>
	void Set(T& data)
	{
		SetPolicyData(data);
	}

	template <IsRenderDataSource T>
	void Set(T* data)
	{
		if (data)
		{
			SetPolicyData(*data);
		}
	}

	template <IsValidRenderData T>
	T* Get()
	{
		return GetPolicyData<T>();
	}

	template <IsValidRenderData T>
	const T* Get() const
	{
		return GetPolicyData<T>();
	}

	template <IsValidRenderData T>
	void DeclareAccess(
		const std::string& passName,
		RenderDataPolicy policy,
		RenderDataAccessMode mode,
		const char* debugName = ""
	)
	{
		RenderDataAccess access;
		access.passName = passName;
		access.typeID = T::StaticRuntimeTypeID();
		access.policy = policy;
		access.mode = mode;
		access.typeName = typeid(T).name();
		access.debugName = debugName;
		m_declaredAccesses.push_back(std::move(access));
	}

	void								 ClearDeclaredAccesses() { m_declaredAccesses.clear(); }
	const std::vector<RenderDataAccess>& GetDeclaredAccesses() const { return m_declaredAccesses; }

	void BeginFrame()
	{
		m_transientData.clear();
	}

	void EndFrame() { m_transientData.clear(); }

	void Release()
	{
		m_transientData.clear();
		m_frameBufferedData.clear();
		m_pingPongHistoryData.clear();
		m_persistentData.clear();
		m_publishedPolicies.clear();
		m_hasPublishedPolicy.clear();
		m_declaredAccesses.clear();
	}

private:
	std::vector<IRenderData*>& SelectRegistry(RenderDataPolicy policy)
	{
		switch (policy)
		{
		case RenderDataPolicy::Transient:
			return m_transientData;
		case RenderDataPolicy::FrameBuffered:
			return m_frameBufferedData;
		case RenderDataPolicy::PingPongHistory:
			return m_pingPongHistoryData;
		case RenderDataPolicy::Persistent:
			return m_persistentData;
		default:
			return m_transientData;
		}
	}

	const std::vector<IRenderData*>& SelectRegistry(RenderDataPolicy policy) const
	{
		switch (policy)
		{
		case RenderDataPolicy::Transient:
			return m_transientData;
		case RenderDataPolicy::FrameBuffered:
			return m_frameBufferedData;
		case RenderDataPolicy::PingPongHistory:
			return m_pingPongHistoryData;
		case RenderDataPolicy::Persistent:
			return m_persistentData;
		default:
			return m_transientData;
		}
	}

	template <IsRenderDataSource T>
	void SetPolicyData(T& data)
	{
		auto&		   registry = SelectRegistry(RenderDataSourceTraits<T>::Policy);
		const uint32_t typeID = RenderDataSourceTraits<T>::TypeID();
		if (typeID >= registry.size())
		{
			registry.resize(typeID + 1);
		}
		if (typeID >= m_publishedPolicies.size())
		{
			m_publishedPolicies.resize(typeID + 1, RenderDataPolicy::Transient);
			m_hasPublishedPolicy.resize(typeID + 1, false);
		}

		registry[typeID] = RenderDataSourceTraits<T>::Current(data);
		m_publishedPolicies[typeID] = RenderDataSourceTraits<T>::Policy;
		m_hasPublishedPolicy[typeID] = true;
	}

	template <IsValidRenderData T>
	T* GetPolicyData()
	{
		const uint32_t typeID = T::StaticRuntimeTypeID();
		if (typeID >= m_hasPublishedPolicy.size() || !m_hasPublishedPolicy[typeID])
		{
			return nullptr;
		}

		auto& registry = SelectRegistry(m_publishedPolicies[typeID]);
		if (typeID >= registry.size() || nullptr == registry[typeID])
		{
			return nullptr;
		}

		return static_cast<T*>(registry[typeID]);
	}

	template <IsValidRenderData T>
	const T* GetPolicyData() const
	{
		const uint32_t typeID = T::StaticRuntimeTypeID();
		if (typeID >= m_hasPublishedPolicy.size() || !m_hasPublishedPolicy[typeID])
		{
			return nullptr;
		}

		const auto& registry = SelectRegistry(m_publishedPolicies[typeID]);
		if (typeID >= registry.size() || nullptr == registry[typeID])
		{
			return nullptr;
		}

		return static_cast<const T*>(registry[typeID]);
	}

	std::vector<IRenderData*>	  m_transientData;
	std::vector<IRenderData*>	  m_frameBufferedData;
	std::vector<IRenderData*>	  m_pingPongHistoryData;
	std::vector<IRenderData*>	  m_persistentData;
	std::vector<RenderDataPolicy> m_publishedPolicies;
	std::vector<bool>			  m_hasPublishedPolicy;
	std::vector<RenderDataAccess> m_declaredAccesses;
};
