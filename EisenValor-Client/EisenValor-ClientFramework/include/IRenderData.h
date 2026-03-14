#pragma once
#include <memory>

using RenderDataTypeID = uint32_t;

namespace RenderTypeInternal
{
inline RenderDataTypeID GetNextTypeID()
{
	static RenderDataTypeID nextID = 0;
	return nextID++;
}

template <typename T>
RenderDataTypeID GetTypeID()
{
	static const RenderDataTypeID typeID = GetNextTypeID();
	return typeID;
}
} // namespace RenderTypeInternal

class IRenderData
{
public:
	virtual ~IRenderData() = default;

	virtual RenderDataTypeID GetRuntimeTypeID() const = 0;
	virtual const char*		 GetTypeName() const = 0;
	virtual bool			 IsValid() const = 0;
};

template <typename Derived>
class RenderDataBase : public IRenderData
{
public:
	RenderDataTypeID		GetRuntimeTypeID() const override { return RenderTypeInternal::GetTypeID<Derived>(); }
	static RenderDataTypeID StaticRuntimeTypeID() { return RenderTypeInternal::GetTypeID<Derived>(); }
	const char*				GetTypeName() const override { return typeid(Derived).name(); }

	virtual bool IsValid() const override { return false; }
};

template <typename T>
concept IsValidRenderData = std::derived_from<T, RenderDataBase<T>>;
