#include "stdafx.h"
#include "GlobalInterfaces.h"

#include "InputGlobal.h"
#include "TimerGlobal.h"

inline void Globals::RegisterAllGlobal()
{
	static bool isCall{};
	if (not isCall) isCall = true; else assert(false && "corruptCall");

	GlobalRegistry::Register<IInputGlobal>("main", InputGlobal::Create());
	GlobalRegistry::Register<ITimerGlobal>("main", TimerGlobal::Create());
	//GlobalRegistry::Register<ISceneGlobal>("main", TimerGlobal::Create());
	//GlobalRegistry::Register<ICollisionGlobal>("main", TimerGlobal::Create());
	//GlobalRegistry::Register<IObejectGlobal>("main", TimerGlobal::Create());

	//GlobalRegistry::Register<IRendererGlobal>("main", RendererGlobal::Create());
	//GlobalRegistry::Register<IDeviceGlobal>("main", DeviceGlobal::Create());
	//GlobalRegistry::Register<ISwapchainGlobal>("main", SwapchainGlobal::Create());
	//GlobalRegistry::Register<IFenceGlobal>("main", FenceGlobal::Create());
	//GlobalRegistry::Register<ICommandQueueGlobal>("main", CommandQueueGlobal::Create());
	//GlobalRegistry::Register<ICommandAllocPoolGlobal>("main", CommandAllocPoolGlobal::Create());
	//GlobalRegistry::Register<IDescriptorHeapGlobal>("main", DescriptorHeapGlobal::Create());
	//GlobalRegistry::Register<IPipelineStateCacheGlobal>("main", PipelineStateCacheGlobal::Create());
	//GlobalRegistry::Register<IRootSignatureCacheGlobal>("main", RootSignatureCacheGlobal::Create());
	//GlobalRegistry::Register<IResourceGlobal>("main", ResourceGlobal::Create());
	//	//texture, shader, binary parsing
}