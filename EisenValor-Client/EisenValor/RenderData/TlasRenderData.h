#pragma once
#include "DxTLAS.h"
#include "RenderDataPolicy.h"
#include <cstdint>
#include <memory>

class TlasRenderData : public RenderDataBase<TlasRenderData>
{
public:
	TlasRenderData() = default;
	virtual ~TlasRenderData() override = default;

	void Initialize(ID3D12Device5* device)
	{
		tlas = std::make_unique<DxTLAS>();
		tlas->Initialize(device);
		lastInstanceCount = 0;
		lastTopologyHash = 0;
		lastTransformHash = 0;
	}

	void Release() override
	{
		tlas.reset();
		lastInstanceCount = 0;
		lastTopologyHash = 0;
		lastTransformHash = 0;
	}

	bool IsValid() const override { return nullptr != tlas; }

	DxTLAS*		  Get() { return tlas.get(); }
	const DxTLAS* Get() const { return tlas.get(); }

	uint32_t lastInstanceCount = 0;
	uint64_t lastTopologyHash = 0;
	uint64_t lastTransformHash = 0;

private:
	std::unique_ptr<DxTLAS> tlas;
};
