#pragma once
#include "AssetFormat.h"
#include <string>
#include <memory>
#include <typeinfo>

using ResourceTypeID = uint32_t;

namespace ResourceTypeInternal
{
inline ResourceTypeID GetNextTypeID()
{
	static ResourceTypeID nextID = 0;
	return nextID++;
}

template <typename T>
ResourceTypeID GetTypeID()
{
	static const ResourceTypeID typeID = GetNextTypeID();
	return typeID;
}
} // namespace ResourceTypeInternal

class IResource
{
public:
	virtual ~IResource() = default;

	virtual const char*	   GetStaticTypeName() const = 0;
	virtual const char*	   GetRuntimeTypeName() const = 0;
	virtual ResourceTypeID GetRuntimeTypeID() const = 0;

	void				 SetGuid(const EvAsset::Guid& guid) { m_guid = guid; }
	const EvAsset::Guid& GetGuid() const { return m_guid; }

	void			   SetName(const std::string& name) { m_name = name; }
	const std::string& GetName() const { return m_name; }

	bool IsReady() const { return m_isReady; }
	void MarkReady(bool ready = true) { m_isReady = ready; }

protected:
	void SetReady(bool ready) { m_isReady = ready; }

	EvAsset::Guid m_guid;
	std::string	  m_name;
	bool		  m_isReady = false;
};

template <typename Derived>
class ResourceBase : public IResource
{
public:
	static constexpr const char* GetStaticResourceName() { return "UnknownResource"; }
	const char*					 GetStaticTypeName() const override { return Derived::GetStaticResourceName(); }
	const char*					 GetRuntimeTypeName() const override { return typeid(Derived).name(); }

	ResourceTypeID		  GetRuntimeTypeID() const override { return ResourceTypeInternal::GetTypeID<Derived>(); }
	static ResourceTypeID StaticRuntimeTypeID() { return ResourceTypeInternal::GetTypeID<Derived>(); }
};

template <typename T>
concept IsValidResource = std::derived_from<T, ResourceBase<T>>;
