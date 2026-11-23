#include "stdafxClient.h"
#include "DxrRenderPass.h"
#include "RaytracingPassCommon.h"
#include "RenderData/RaytracingFrameRenderData.h"
#include <DxDeviceGlobal.h>
#include <RenderContext.h>

void DxrRenderPass::Initialize()
{
	ComPtr<ID3D12Device5> device;
	ThrowIfFailed(GLOBAL(DxDeviceGlobal).GetDevice()->QueryInterface(IID_PPV_ARGS(&device)));
	m_pipeline = BuildRaytracingPipeline(device.Get(), L"Resource/Shader/RaytracingLibrary.hlsl", 4, 16, false);
	m_shaderTable = BuildRaytracingShaderTable(device.Get(), m_pipeline.get(), "RaytracingLibrary_ShaderTable");
}

void DxrRenderPass::Release()
{
	m_shaderTable.reset();
	m_pipeline.reset();
}

void DxrRenderPass::DeclareRenderData(RenderContext* renderContext)
{
	DeclareRaytracingInputs(renderContext, GetName());
}

bool DxrRenderPass::ShouldExecute(const RenderContext* renderContext) const
{
	if (!renderContext || !m_pipeline || !m_shaderTable)
	{
		return false;
	}
	const auto* frameData = renderContext->Get<RaytracingFrameRenderData>();
	return frameData && frameData->ready && frameData->path == RaytracingPath::Realtime;
}

void DxrRenderPass::Execute(DxFrameResource* frame, Scene* scene, RenderContext* renderContext)
{
	if (!frame || !renderContext)
	{
		return;
	}
	auto commands = BindRaytracingResources(frame, renderContext, m_pipeline.get());
	if (!commands)
	{
		return;
	}
	DispatchRaytracing(frame, renderContext, commands.Get(), *m_shaderTable);
}
