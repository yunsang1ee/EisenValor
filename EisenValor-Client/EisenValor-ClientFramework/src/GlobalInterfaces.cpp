#include "stdafxClientFramework.h"
#include "InputGlobal.h"
#include "TimerGlobal.h"
#include "SceneGlobal.h"
#include "DxDeviceGlobal.h"
#include "DxCommandQueueGlobal.h"
#include "DxShaderCompilerGlobal.h"
#include "DxGarbageCollectorGlobal.h"
#include "DxDescriptorHeapGlobal.h"

#ifdef _DEBUG
#include "DxDebugGlobal.h"
#endif

namespace Globals
{

void Initialize()
{
	MANAGER(InputGlobal).Initialize();
	MANAGER(TimerGlobal).Initialize();

#ifdef _DEBUG
	auto& debugG = MANAGER(DxDebugGlobal);
	debugG.Initialize();
#endif
	auto& deviceG = MANAGER(DxDeviceGlobal);
	deviceG.Initialize();
#ifdef _DEBUG
	debugG.SetupDebugMessages(deviceG.GetDevice());
	debugG.SetBreakOnSeverity(true, true);
#endif
	MANAGER(DxGarbageCollectorGlobal).Initialize();
	MANAGER(DxShaderCompilerGlobal).Initialize();
	auto* device = deviceG.GetDevice();
	MANAGER(DxDescriptorHeapGlobal).Initialize(device, 1'000'000);
	MANAGER(DxGfxCommandQueueGlobal).Initialize(device);

	MANAGER(SceneGlobal).Initialize();
}

void Shutdown()
{
	MANAGER(SceneGlobal).Release();

	MANAGER(DxGfxCommandQueueGlobal).Release();
	MANAGER(DxDescriptorHeapGlobal).Release();
	MANAGER(DxShaderCompilerGlobal).Release();
	MANAGER(DxGarbageCollectorGlobal).Release();
#ifdef _DEBUG
	auto& debugG = MANAGER(DxDebugGlobal);
	debugG.PrintDebugMessages();
	debugG.Release();
#endif
	MANAGER(DxDeviceGlobal).Release();

	MANAGER(TimerGlobal).Release();
	MANAGER(InputGlobal).Release();
}

} // namespace Globals