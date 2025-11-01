#include "stdafxClientFramework.h"
#include "InputGlobal.h"
#include "TimerGlobal.h"
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
	MANAGER(DxDebugGlobal).Initialize();
#endif
	MANAGER(DxDeviceGlobal).Initialize();
	MANAGER(DxGarbageCollectorGlobal).Initialize();
	MANAGER(DxShaderCompilerGlobal).Initialize();
	auto* device = MANAGER(DxDeviceGlobal).GetDevice();
	MANAGER(DxDescriptorHeapGlobal).Initialize(device, 1'000'000);
	MANAGER(DxGfxCommandQueueGlobal).Initialize(device);
}

void Shutdown()
{
	MANAGER(DxGfxCommandQueueGlobal).Release();
	MANAGER(DxDescriptorHeapGlobal).Release();
	MANAGER(DxShaderCompilerGlobal).Release();
	MANAGER(DxGarbageCollectorGlobal).Release();
	MANAGER(DxDeviceGlobal).Release();
#ifdef _DEBUG
	MANAGER(DxDebugGlobal).Release();
#endif

	MANAGER(TimerGlobal).Release();
	MANAGER(InputGlobal).Release();
}

} // namespace Globals