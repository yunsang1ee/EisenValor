#include "stdafxClientFramework.h"
#include "InputGlobal.h"
#include "TimerGlobal.h"
#include "SceneGlobal.h"
#include "DxDeviceGlobal.h"
#include "DxCommandQueueGlobal.h"
#include "DxShaderCompilerGlobal.h"
#include "DxGarbageCollectorGlobal.h"
#include "DxDescriptorHeapGlobal.h"
#include "DxSamplerHeapGlobal.h"
#include "ResourceGlobal.h"
#include "DxRendererGlobal.h"

#ifdef _DEBUG
#include "DxDebugGlobal.h"
#endif

namespace Globals
{

void Initialize(HWND hwnd)
{
	GLOBAL(InputGlobal).Initialize(hwnd);
	GLOBAL(TimerGlobal).Initialize();

#ifdef _DEBUG
	auto& debugG = GLOBAL(DxDebugGlobal);
	debugG.Initialize();
#endif
	auto& deviceG = GLOBAL(DxDeviceGlobal);
	deviceG.Initialize();
#ifdef _DEBUG
	debugG.SetupDebugMessages(deviceG.GetDevice());
	debugG.SetBreakOnSeverity(true, false);
#endif
	GLOBAL(DxGarbageCollectorGlobal).Initialize();
	GLOBAL(DxShaderCompilerGlobal).Initialize();
	auto* device = deviceG.GetDevice();
	GLOBAL(DxDescriptorHeapGlobal).Initialize(device, 1'000'000);
	GLOBAL(DxSamplerHeapGlobal).Initialize(device, 2048);

	// 기본 샘플러 생성 (Index 0)
	D3D12_SAMPLER_DESC defaultSamplerDesc = {};
	defaultSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	defaultSamplerDesc.AddressU = defaultSamplerDesc.AddressV = defaultSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	defaultSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	defaultSamplerDesc.MinLOD = 0;
	defaultSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	GLOBAL(DxSamplerHeapGlobal).CreateSampler(device, defaultSamplerDesc);

	GLOBAL(DxGfxCommandQueueGlobal).Initialize(device);
	GLOBAL(ResourceGlobal).Initialize();
	GLOBAL(DxRendererGlobal).Initialize();

	GLOBAL(SceneGlobal).Initialize();
}

void Shutdown()
{
	GLOBAL(SceneGlobal).Release();

	GLOBAL(DxRendererGlobal).Release();
	GLOBAL(ResourceGlobal).Release();
	GLOBAL(DxGfxCommandQueueGlobal).Release();
	GLOBAL(DxSamplerHeapGlobal).Release();
	GLOBAL(DxDescriptorHeapGlobal).Release();
	GLOBAL(DxShaderCompilerGlobal).Release();
	GLOBAL(DxGarbageCollectorGlobal).Release();
#ifdef _DEBUG
	auto& debugG = GLOBAL(DxDebugGlobal);
	debugG.PrintDebugMessages();
	debugG.Release();
#endif
	GLOBAL(DxDeviceGlobal).Release();

	GLOBAL(TimerGlobal).Release();
	GLOBAL(InputGlobal).Release();
}

} // namespace Globals
