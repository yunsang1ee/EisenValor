#pragma once
#include <DxPipelineState.h>
#include <IRenderPass.h>
#include <RenderDataPolicy.h>
#include <cstdint>
#include <memory>
#include "RenderData/RestirCandidateRenderData.h"
#include "RenderData/RestirFinalReservoirRenderData.h"
#include "RenderData/RestirTemporalHistoryRenderData.h"

class DxBuffer;
class GeoTableRenderData;
class InstanceRenderData;
class TlasRenderData;

class RestirTemporalReusePass : public IRenderPass
{
public:
	RestirTemporalReusePass(uint32_t width, uint32_t height);
	~RestirTemporalReusePass() override = default;

	void		Initialize() override;
	void		Release() override;
	void		DeclareRenderData(RenderContext* renderContext) override;
	void		Execute(DxFrameResource* frame, Scene* scene, RenderContext* renderContext) override;
	void		OnResize(uint32_t width, uint32_t height) override;
	const char* GetName() const override { return "RestirTemporalReuse"; }

private:
	void CreatePipeline();
	void CreateHistoryResources(uint32_t width, uint32_t height);
	bool DispatchTemporalReuse(
		ID3D12GraphicsCommandList*		cmdList,
		RestirCandidateRenderData*		candidateData,
		TlasRenderData*					tlasData,
		InstanceRenderData*				instanceData,
		GeoTableRenderData*				geoTableData,
		RestirFinalReservoirRenderData* finalReservoirData
	);
	void PublishInitialReservoir(
		RenderContext*					renderContext,
		RestirCandidateRenderData*		candidateData,
		RestirFinalReservoirRenderData* finalReservoirData
	);

private:
	ComPtr<ID3D12RootSignature> m_rootSignature;
	DxPipelineState				m_pipelineState;

	PingPongHistory<RestirTemporalHistoryRenderData> m_temporalHistory{HistorySwapMode::Manual};
	Transient<RestirFinalReservoirRenderData>		 m_finalReservoirData;
	std::shared_ptr<DxBuffer>						 m_finalReservoirBuffer;

	uint32_t m_width = 0;
	uint32_t m_height = 0;
	uint32_t m_frameSeed = 0;
	bool	 m_initialized = false;
	bool	 m_historyValid = false;
};
