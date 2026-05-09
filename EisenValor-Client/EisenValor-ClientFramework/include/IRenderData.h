#pragma once
#include <cstdint>
#include <typeinfo>

using RenderDataTypeID = uint32_t;

enum class RenderDataPolicy : uint8_t
{
	Transient,
	FrameBuffered,
	PingPongHistory,
	Persistent
};

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

	virtual void BeginFrame(uint32_t frameIndex) {}
	virtual void EndFrame() {}
	virtual void OnResize(uint32_t width, uint32_t height) {}
	virtual void ResetHistory() {}
	virtual void Release() {}
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
