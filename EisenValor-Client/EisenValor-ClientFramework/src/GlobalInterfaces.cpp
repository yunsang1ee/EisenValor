#include "stdafxClientFramework.h"
#include "GlobalInterfaces.h"

#include <InputGlobal.h>
#include <TimerGlobal.h>
#include <DxDebugGlobal.h>
#include <DxDeviceGlobal.h>
#include <DxCommandQueueGlobal.h>

void Globals::InitializeGlobalRegistry()
{
	static bool isCall{};
	if (not isCall) isCall = true; else assert(false && "corruptCall");

	using GR = GlobalRegistry;

	GR::Register<IInputGlobal>	(GR::RegistryType::Main, InputGlobal::Create());
	GR::Register<ITimerGlobal>	(GR::RegistryType::Main, TimerGlobal::Create());

#ifdef _DEBUG
	GR::Register<IDxDebugGlobal>	(GR::RegistryType::Main, DxDebugGlobal::Create());
#endif //_DEBUG

	GR::Register<IDxDeviceGlobal> (GR::RegistryType::Main, DxDeviceGlobal::Create());
	auto& device = GlobalRegistry::Get<IDxDeviceGlobal>();
	device.Initialize();

	GR::Register<IDxGraphicsCommandQueueGlobal>	(GR::RegistryType::Main, DxGraphicsCommandQueueGlobal::Create());
	//GR::Register<IDxComputeCommandQueueGlobal>	(GR::RegistryType::Main, DxComputeCommandQueueGlobal::Create());
	//GR::Register<IDxCopyCommandQueueGlobal>		(GR::RegistryType::Main, DxCopyCommandQueueGlobal::Create());

	GlobalRegistry::Get<IDxGraphicsCommandQueueGlobal>().Initialize(device.GetDevice());

	//GR::Register<IRendererGlobal>				(GR::RegistryType::Main, RendererGlobal::Create());
	//GR::Register<ISwapchainGlobal>			(GR::RegistryType::Main, SwapchainGlobal::Create());
	//GR::Register<IFenceGlobal>				(GR::RegistryType::Main, FenceGlobal::Create());
	//GR::Register<ICommandAllocPoolGlobal>		(GR::RegistryType::Main, CommandAllocPoolGlobal::Create());
	//GR::Register<IDescriptorHeapGlobal>		(GR::RegistryType::Main, DescriptorHeapGlobal::Create());
	//GR::Register<IPipelineStateCacheGlobal>	(GR::RegistryType::Main, PipelineStateCacheGlobal::Create());
	//GR::Register<IRootSignatureCacheGlobal>	(GR::RegistryType::Main, RootSignatureCacheGlobal::Create());
	//GR::Register<IResourceGlobal>				(GR::RegistryType::Main, ResourceGlobal::Create());
	//GR::Register<ISceneGlobal>				(GR::RegistryType::Main, ISceneGlobal::Create());
	//GR::Register<ICollisionGlobal>			(GR::RegistryType::Main, ICollisionGlobal::Create());
	//GR::Register<IObejectGlobal>				(GR::RegistryType::Main, IObejectGlobal::Create());
	//	//texture, shader, binary parsing
}