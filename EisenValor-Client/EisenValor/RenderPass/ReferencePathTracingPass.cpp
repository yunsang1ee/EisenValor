#include "stdafxClient.h"
#include "ReferencePathTracingPass.h"
#include "RaytracingPassCommon.h"
#include "RenderData/RaytracingFrameRenderData.h"
#include <DxDeviceGlobal.h>
#include <RenderContext.h>

void ReferencePathTracingPass::Initialize()
{
	ComPtr<ID3D12Device5> device;
	ThrowIfFailed(GLOBAL(DxDeviceGlobal).GetDevice()->QueryInterface(IID_PPV_ARGS(&device)));
	m_pipeline = BuildRaytracingPipeline(device.Get(), L"Resource/Shader/RaytracingLibraryPT.hlsl", 19, 16, false);
	m_shaderTable = BuildRaytracingShaderTable(device.Get(), m_pipeline.get(), "RaytracingLibraryPT_ShaderTable");
}

void ReferencePathTracingPass::Release()
{
	m_shaderTable.reset();
	m_pipeline.reset();
}

void ReferencePathTracingPass::DeclareRenderData(RenderContext* renderContext)
{
	DeclareRaytracingInputs(renderContext, GetName());
}

void ReferencePathTracingPass::Execute(DxFrameResource* frame, Scene* scene, RenderContext* renderContext)
{
	if (!frame || !renderContext || !m_pipeline || !m_shaderTable)
	{
		return;
	}
	auto* frameData = renderContext->Get<RaytracingFrameRenderData>();
	if (!frameData || !frameData->ready || frameData->path != RaytracingPath::Reference)
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
