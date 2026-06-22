#pragma once
#include <DxPipelineState.h>
#include <IRenderPass.h>
#include <RenderDataPolicy.h>
#include <DirectXMath.h>
#include <cstdint>
#include <memory>
#include "RenderData/RestirCandidateRenderData.h"
#include "RenderData/RestirFinalReservoirRenderData.h"
#include "RenderData/RestirTemporalHistoryRenderData.h"

class DxBuffer;
class CameraRenderData;
class GeoTableRenderData;
class InstanceRenderData;
class MaterialRenderData;
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
	RenderResolutionDomain GetResolutionDomain() const override { return RenderResolutionDomain::Render; }
	const char* GetName() const override { return "RestirTemporalReuse"; }

private:
	void CreatePipeline();
	void CreateHistoryResources(uint32_t width, uint32_t height);
	bool DispatchTemporalReuse(
		ID3D12GraphicsCommandList*		cmdList,
		CameraRenderData*				cameraData,
		RestirCandidateRenderData*		candidateData,
		TlasRenderData*					tlasData,
		InstanceRenderData*				instanceData,
		MaterialRenderData*				materialData,
		GeoTableRenderData*				geoTableData,
		RestirFinalReservoirRenderData* finalReservoirData
	);
	void PublishInitialReservoir(
		RenderContext*					renderContext,
		RestirCandidateRenderData*		candidateData,
		RestirFinalReservoirRenderData* finalReservoirData
	);
	bool ShouldResetHistory(
		const CameraRenderData& cameraData, const RestirCandidateRenderData& candidateData
	) const;
	void RecordHistoryValidationState(
		const CameraRenderData& cameraData, const RestirCandidateRenderData& candidateData
	);
	void InvalidateHistory();

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
	bool	 m_hasHistoryValidationState = false;
	uint64_t m_lastHistorySignature = 0;
	DirectX::XMFLOAT3 m_lastCameraPosition = {0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT3 m_lastCameraDirection = {0.0f, 0.0f, 1.0f};
	float m_lastCameraNearZ = 0.0f;
	float m_lastCameraFarZ = 0.0f;
	float m_lastCameraFov = 0.0f;
	float m_lastCameraAspectRatio = 0.0f;
};
