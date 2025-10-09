#include "stdafxClientFramework.h"
#include "InputGlobal.h"
#include "TimerGlobal.h"
#include "DxDeviceGlobal.h"
#include "DxCommandQueueGlobal.h"
#include "DxShaderCompilerGlobal.h"

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
	MANAGER(DxShaderCompilerGlobal).Initialize();
	MANAGER(DxGfxCommandQueueGlobal).Initialize(MANAGER(DxDeviceGlobal).GetDevice());
}

void Shutdown()
{
	MANAGER(DxGfxCommandQueueGlobal).Release();
	MANAGER(DxShaderCompilerGlobal).Release();
	MANAGER(DxDeviceGlobal).Release();
#ifdef _DEBUG
	MANAGER(DxDebugGlobal).Release();
#endif

	MANAGER(TimerGlobal).Release();
	MANAGER(InputGlobal).Release();
}

} // namespace Globals