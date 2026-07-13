#include "stdafxClientFramework.h"
#include "StreamlineGlobal.h"

#include <sl.h>
#include <sl_dlss.h>
#include <sl_dlss_d.h>
#include <sl_helpers.h>

#include <algorithm>

namespace
{
const sl::ViewportHandle kMainViewport{0u};

sl::DLSSMode ToDLSSMode(StreamlineQualityMode mode)
{
	switch (mode)
	{
	case StreamlineQualityMode::DLAA:
		return sl::DLSSMode::eDLAA;
	case StreamlineQualityMode::Balanced:
		return sl::DLSSMode::eBalanced;
	case StreamlineQualityMode::Performance:
		return sl::DLSSMode::eMaxPerformance;
	case StreamlineQualityMode::Quality:
	default:
		return sl::DLSSMode::eMaxQuality;
	}
}

sl::float4x4 ToStreamlineMatrix(DirectX::FXMMATRIX matrix)
{
	DirectX::XMFLOAT4X4 stored;
	DirectX::XMStoreFloat4x4(&stored, matrix);

	sl::float4x4 result = {};
	result.row[0] = {stored._11, stored._12, stored._13, stored._14};
	result.row[1] = {stored._21, stored._22, stored._23, stored._24};
	result.row[2] = {stored._31, stored._32, stored._33, stored._34};
	result.row[3] = {stored._41, stored._42, stored._43, stored._44};
	return result;
}

void StreamlineLogCallback(sl::LogType type, const char* message)
{
	if (nullptr == message)
	{
		return;
	}

	switch (type)
	{
	case sl::LogType::eError:
		GRAPHICS_LOG_FMT("[Streamline][ERROR] {}\n", message);
		break;
	case sl::LogType::eWarn:
		GRAPHICS_LOG_FMT("[Streamline][WARN] {}\n", message);
		break;
	default:
#if defined(_DEBUG) || defined(ENABLE_DEBUG_LOG)
		GRAPHICS_LOG_FMT("[Streamline] {}\n", message);
#endif
		break;
	}
}

bool CheckResult(sl::Result result, const char* operation)
{
	if (sl::Result::eOk == result)
	{
		return true;
	}

	GRAPHICS_LOG_FMT("[Streamline] {} failed (result={})\n", operation, static_cast<int32_t>(result));
	return false;
}
} // namespace

void StreamlineGlobal::Initialize()
{
	if (m_initialized)
	{
		return;
	}

	static constexpr sl::Feature features[] = {sl::kFeatureDLSS, sl::kFeatureDLSS_RR};

	sl::Preferences preferences = {};
#if defined(_DEBUG) || defined(ENABLE_DEBUG_LOG)
	preferences.showConsole = true;
#endif
	preferences.logLevel = sl::LogLevel::eDefault;
	preferences.logMessageCallback = StreamlineLogCallback;
	preferences.flags = sl::PreferenceFlags::eDisableCLStateTracking |
						sl::PreferenceFlags::eUseFrameBasedResourceTagging;
	preferences.featuresToLoad = features;
	preferences.numFeaturesToLoad = static_cast<uint32_t>(std::size(features));
	preferences.engine = sl::EngineType::eCustom;
	preferences.engineVersion = "1.0.0";
	preferences.projectId = "d9b460c9-7e4c-4f46-b9d5-fed6920bec21";
	preferences.renderAPI = sl::RenderAPI::eD3D12;

	const sl::Result result = slInit(preferences, sl::kSDKVersion);
	if (!CheckResult(result, "slInit"))
	{
		return;
	}

	DirectX::XMStoreFloat4x4(&m_previousViewProjection, DirectX::XMMatrixIdentity());
	m_initialized = true;
	GRAPHICS_LOG_FMT("[Streamline] Initialized SDK {}.{}.{}\n", SL_VERSION_MAJOR, SL_VERSION_MINOR, SL_VERSION_PATCH);
}

void StreamlineGlobal::Release()
{
	if (!m_initialized)
	{
		return;
	}

	CheckResult(slShutdown(), "slShutdown");
	m_initialized = false;
	m_deviceReady = false;
	m_dlssSupported = false;
	m_rayReconstructionSupported = false;
	m_hasPreviousViewProjection = false;
	GRAPHICS_LOG_FMT("[Streamline] Released\n");
}

void StreamlineGlobal::SetD3DDevice(ID3D12Device* device, IDXGIAdapter* adapter)
{
	if (!m_initialized || nullptr == device || nullptr == adapter)
	{
		return;
	}

	if (!CheckResult(slSetD3DDevice(device), "slSetD3DDevice"))
	{
		return;
	}

	DXGI_ADAPTER_DESC adapterDesc = {};
	if (FAILED(adapter->GetDesc(&adapterDesc)))
	{
		GRAPHICS_LOG_FMT("[Streamline] Failed to query adapter LUID\n");
		return;
	}

	sl::AdapterInfo adapterInfo = {};
	adapterInfo.deviceLUID = reinterpret_cast<uint8_t*>(&adapterDesc.AdapterLuid);
	adapterInfo.deviceLUIDSizeInBytes = sizeof(adapterDesc.AdapterLuid);

	m_dlssSupported = sl::Result::eOk == slIsFeatureSupported(sl::kFeatureDLSS, adapterInfo);
	m_rayReconstructionSupported = sl::Result::eOk == slIsFeatureSupported(sl::kFeatureDLSS_RR, adapterInfo);
	m_deviceReady = true;

	GRAPHICS_LOG_FMT(
		"[Streamline] Feature support: DLSS={}, RayReconstruction={}\n", m_dlssSupported,
		m_rayReconstructionSupported
	);
}

StreamlineResolution StreamlineGlobal::GetOptimalResolution(uint32_t displayWidth, uint32_t displayHeight) const
{
	StreamlineResolution resolution = {displayWidth, displayHeight, displayWidth, displayHeight};
	if (!IsEnabled() || 0u == displayWidth || 0u == displayHeight)
	{
		return resolution;
	}

	const sl::DLSSMode mode = ToDLSSMode(m_qualityMode);
	if (sl::DLSSMode::eDLAA == mode)
	{
		return resolution;
	}

	if (IsRayReconstructionEnabled())
	{
		sl::DLSSDOptions options = {};
		options.mode = mode;
		options.outputWidth = displayWidth;
		options.outputHeight = displayHeight;
		options.colorBuffersHDR = sl::Boolean::eTrue;

		sl::DLSSDOptimalSettings settings = {};
		if (sl::Result::eOk == slDLSSDGetOptimalSettings(options, settings) && settings.optimalRenderWidth > 0u &&
			settings.optimalRenderHeight > 0u)
		{
			resolution.renderWidth = settings.optimalRenderWidth;
			resolution.renderHeight = settings.optimalRenderHeight;
			return resolution;
		}
	}

	sl::DLSSOptions options = {};
	options.mode = mode;
	options.outputWidth = displayWidth;
	options.outputHeight = displayHeight;
	options.colorBuffersHDR = sl::Boolean::eTrue;

	sl::DLSSOptimalSettings settings = {};
	if (sl::Result::eOk == slDLSSGetOptimalSettings(options, settings) && settings.optimalRenderWidth > 0u &&
		settings.optimalRenderHeight > 0u)
	{
		resolution.renderWidth = settings.optimalRenderWidth;
		resolution.renderHeight = settings.optimalRenderHeight;
	}
	return resolution;
}

bool StreamlineGlobal::Evaluate(const StreamlineEvaluateDesc& desc)
{
	if (!IsEnabled() || !m_deviceReady || nullptr == desc.commandList || nullptr == desc.colorInput ||
		nullptr == desc.colorOutput || nullptr == desc.linearDepth || nullptr == desc.motionVectors ||
		nullptr == desc.camera)
	{
		return false;
	}

	const bool useRayReconstruction = IsRayReconstructionEnabled() && nullptr != desc.diffuseAlbedo &&
								  nullptr != desc.specularAlbedo && nullptr != desc.normalRoughness;
	const sl::DLSSMode mode = ToDLSSMode(m_qualityMode);

	sl::DLSSOptions dlssOptions = {};
	dlssOptions.mode = mode;
	dlssOptions.outputWidth = desc.displayWidth;
	dlssOptions.outputHeight = desc.displayHeight;
	dlssOptions.colorBuffersHDR = sl::Boolean::eTrue;
	dlssOptions.useAutoExposure = sl::Boolean::eTrue;
	dlssOptions.alphaUpscalingEnabled = sl::Boolean::eFalse;
	dlssOptions.dlaaPreset = sl::DLSSPreset::ePresetK;
	dlssOptions.qualityPreset = sl::DLSSPreset::ePresetK;
	dlssOptions.balancedPreset = sl::DLSSPreset::ePresetK;
	dlssOptions.performancePreset = sl::DLSSPreset::ePresetM;
	dlssOptions.ultraPerformancePreset = sl::DLSSPreset::ePresetL;
	if (!CheckResult(slDLSSSetOptions(kMainViewport, dlssOptions), "slDLSSSetOptions"))
	{
		return false;
	}

	const DirectX::XMMATRIX view = desc.camera->viewMatrix;
	const DirectX::XMMATRIX projection = desc.camera->projectionMatrix;
	const DirectX::XMMATRIX viewProjection = DirectX::XMMatrixMultiply(view, projection);
	const DirectX::XMMATRIX inverseView = DirectX::XMMatrixInverse(nullptr, view);
	const DirectX::XMMATRIX inverseProjection = DirectX::XMMatrixInverse(nullptr, projection);
	const DirectX::XMMATRIX previousViewProjection = m_hasPreviousViewProjection
											   ? DirectX::XMLoadFloat4x4(&m_previousViewProjection)
											   : viewProjection;
	const DirectX::XMMATRIX clipToPreviousClip =
		DirectX::XMMatrixMultiply(DirectX::XMMatrixInverse(nullptr, viewProjection), previousViewProjection);
	const DirectX::XMMATRIX previousClipToClip = DirectX::XMMatrixInverse(nullptr, clipToPreviousClip);

	if (useRayReconstruction)
	{
		sl::DLSSDOptions rrOptions = {};
		rrOptions.mode = mode;
		rrOptions.outputWidth = desc.displayWidth;
		rrOptions.outputHeight = desc.displayHeight;
		rrOptions.colorBuffersHDR = sl::Boolean::eTrue;
		rrOptions.normalRoughnessMode = sl::DLSSDNormalRoughnessMode::ePacked;
		rrOptions.worldToCameraView = ToStreamlineMatrix(view);
		rrOptions.cameraViewToWorld = ToStreamlineMatrix(inverseView);
		rrOptions.alphaUpscalingEnabled = sl::Boolean::eFalse;
		rrOptions.dlaaPreset = sl::DLSSDPreset::ePresetD;
		rrOptions.qualityPreset = sl::DLSSDPreset::ePresetD;
		rrOptions.balancedPreset = sl::DLSSDPreset::ePresetD;
		rrOptions.performancePreset = sl::DLSSDPreset::ePresetD;
		rrOptions.ultraPerformancePreset = sl::DLSSDPreset::ePresetD;
		if (!CheckResult(slDLSSDSetOptions(kMainViewport, rrOptions), "slDLSSDSetOptions"))
		{
			return false;
		}
	}

	sl::FrameToken* frameToken = nullptr;
	if (!CheckResult(slGetNewFrameToken(frameToken, &desc.frameIndex), "slGetNewFrameToken") || nullptr == frameToken)
	{
		return false;
	}

	sl::Constants constants = {};
	constants.cameraViewToClip = ToStreamlineMatrix(projection);
	constants.clipToCameraView = ToStreamlineMatrix(inverseProjection);
	constants.clipToPrevClip = ToStreamlineMatrix(clipToPreviousClip);
	constants.prevClipToClip = ToStreamlineMatrix(previousClipToClip);
	constants.jitterOffset = {desc.camera->jitterPixels.x, desc.camera->jitterPixels.y};
	constants.mvecScale = {1.0f, 1.0f};
	constants.cameraPos = {
		desc.camera->cameraPosition.x, desc.camera->cameraPosition.y, desc.camera->cameraPosition.z};
	DirectX::XMFLOAT4X4 inverseViewStored;
	DirectX::XMStoreFloat4x4(&inverseViewStored, inverseView);
	constants.cameraRight = {inverseViewStored._11, inverseViewStored._12, inverseViewStored._13};
	constants.cameraUp = {inverseViewStored._21, inverseViewStored._22, inverseViewStored._23};
	constants.cameraFwd = {inverseViewStored._31, inverseViewStored._32, inverseViewStored._33};
	constants.cameraNear = desc.camera->nearZ;
	constants.cameraFar = desc.camera->farZ;
	constants.cameraFOV = desc.camera->fov;
	constants.cameraAspectRatio = desc.camera->aspectRatio;
	constants.depthInverted = sl::Boolean::eFalse;
	constants.cameraMotionIncluded = sl::Boolean::eTrue;
	constants.motionVectors3D = sl::Boolean::eFalse;
	constants.reset = (desc.reset || m_historyResetRequested || !m_hasPreviousViewProjection) ? sl::Boolean::eTrue
																			  : sl::Boolean::eFalse;
	constants.orthographicProjection = sl::Boolean::eFalse;
	constants.motionVectorsDilated = sl::Boolean::eFalse;
	constants.motionVectorsJittered = sl::Boolean::eFalse;
	if (!CheckResult(slSetConstants(constants, *frameToken, kMainViewport), "slSetConstants"))
	{
		return false;
	}

	const sl::Extent inputExtent = {0u, 0u, desc.renderWidth, desc.renderHeight};
	const sl::Extent outputExtent = {0u, 0u, desc.displayWidth, desc.displayHeight};
	constexpr uint32_t inputState = static_cast<uint32_t>(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	constexpr uint32_t outputState = static_cast<uint32_t>(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	sl::Resource colorInput(sl::ResourceType::eTex2d, desc.colorInput, inputState);
	sl::Resource colorOutput(sl::ResourceType::eTex2d, desc.colorOutput, outputState);
	sl::Resource linearDepth(sl::ResourceType::eTex2d, desc.linearDepth, inputState);
	sl::Resource motionVectors(sl::ResourceType::eTex2d, desc.motionVectors, inputState);
	sl::Resource diffuseAlbedo(sl::ResourceType::eTex2d, desc.diffuseAlbedo, inputState);
	sl::Resource specularAlbedo(sl::ResourceType::eTex2d, desc.specularAlbedo, inputState);
	sl::Resource normalRoughness(sl::ResourceType::eTex2d, desc.normalRoughness, inputState);

	sl::ResourceTag tags[] = {
		{&colorInput, sl::kBufferTypeScalingInputColor, sl::ResourceLifecycle::eOnlyValidNow, &inputExtent},
		{&colorOutput, sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eOnlyValidNow, &outputExtent},
		{&linearDepth, sl::kBufferTypeLinearDepth, sl::ResourceLifecycle::eOnlyValidNow, &inputExtent},
		{&motionVectors, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eOnlyValidNow, &inputExtent},
		{&diffuseAlbedo, sl::kBufferTypeAlbedo, sl::ResourceLifecycle::eOnlyValidNow, &inputExtent},
		{&specularAlbedo, sl::kBufferTypeSpecularAlbedo, sl::ResourceLifecycle::eOnlyValidNow, &inputExtent},
		{&normalRoughness, sl::kBufferTypeNormalRoughness, sl::ResourceLifecycle::eOnlyValidNow, &inputExtent}};
	const uint32_t tagCount = useRayReconstruction ? static_cast<uint32_t>(std::size(tags)) : 4u;
	if (!CheckResult(
			slSetTagForFrame(*frameToken, kMainViewport, tags, tagCount, desc.commandList), "slSetTagForFrame"
		))
	{
		return false;
	}

	const sl::BaseStructure* evaluateInputs[] = {&kMainViewport};
	const sl::Feature feature = useRayReconstruction ? sl::kFeatureDLSS_RR : sl::kFeatureDLSS;
	const bool evaluated = CheckResult(
		slEvaluateFeature(feature, *frameToken, evaluateInputs, static_cast<uint32_t>(std::size(evaluateInputs)),
			desc.commandList),
		"slEvaluateFeature"
	);

	DirectX::XMStoreFloat4x4(&m_previousViewProjection, viewProjection);
	m_hasPreviousViewProjection = true;
	if (evaluated)
	{
		m_historyResetRequested = false;
	}
	return evaluated;
}

void StreamlineGlobal::SetEnabled(bool enabled)
{
	if (m_enabled != enabled)
	{
		m_enabled = enabled;
		RequestHistoryReset();
	}
}

void StreamlineGlobal::SetPreferRayReconstruction(bool enabled)
{
	if (m_preferRayReconstruction != enabled)
	{
		m_preferRayReconstruction = enabled;
		RequestHistoryReset();
	}
}

void StreamlineGlobal::SetQualityMode(StreamlineQualityMode mode)
{
	if (m_qualityMode != mode)
	{
		m_qualityMode = mode;
		RequestHistoryReset();
	}
}
