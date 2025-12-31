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

	enum class Lifetime
	{
		Transient,	// 프레임 내에서만 유효
		Persistent, // 프레임 간 유지 (N-1 프레임)
		Temporal	// 여러 프레임 유지 (N-1, N-2, ...)
	};

	virtual Lifetime GetLifetime() const = 0;
};

template <typename Derived>
class RenderDataBase : public IRenderData
{
public:
	RenderDataTypeID		GetRuntimeTypeID() const override { return RenderTypeInternal::GetTypeID<Derived>(); }
	static RenderDataTypeID StaticRuntimeTypeID() { return RenderTypeInternal::GetTypeID<Derived>(); }
	const char*				GetTypeName() const override { return typeid(Derived).name(); }
};

template <typename T>
concept IsValidRenderData = std::derived_from<T, RenderDataBase<T>>;
