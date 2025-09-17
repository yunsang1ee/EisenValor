#include "stdafxClientFramework.h"
#include "DxFeatureCaps.h"

constexpr D3D_SHADER_MODEL SHADER_MODELS[] = {
	D3D_SHADER_MODEL_6_7, // 최신
	D3D_SHADER_MODEL_6_6, D3D_SHADER_MODEL_6_5, D3D_SHADER_MODEL_6_4, D3D_SHADER_MODEL_6_3,
	D3D_SHADER_MODEL_6_2, D3D_SHADER_MODEL_6_1, D3D_SHADER_MODEL_6_0,
	D3D_SHADER_MODEL_5_1 // 최소 지원
};

DxFeatureCaps DxFeatureCaps::Query(ID3D12Device* device, IDXGIAdapter4* adapter)
{
	DxFeatureCaps caps = {};

	if (!device || !adapter)
	{
		DEBUG_LOG_FMT("[DxFeatureCaps] Warning: Null device or adapter provided\n");
		return caps;
	}

	// ===== 기본 D3D12 기능 쿼리 =====

	// D3D12 Options
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &caps.options, sizeof(caps.options))))
	{
		DEBUG_LOG_FMT("[DxFeatureCaps] Warning: Failed to query D3D12 options\n");
	}

	// Root Signature 버전
	D3D_ROOT_SIGNATURE_VERSION rootSignatureVersions[] = {
		D3D_ROOT_SIGNATURE_VERSION_1_2, D3D_ROOT_SIGNATURE_VERSION_1_1, D3D_ROOT_SIGNATURE_VERSION_1_0
	};

	caps.rootSignature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;

	for (auto version : rootSignatureVersions)
	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE tempRootSig = {};
		tempRootSig.HighestVersion = version;

		if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &tempRootSig, sizeof(tempRootSig))))
		{
			caps.rootSignature.HighestVersion = tempRootSig.HighestVersion;
			DEBUG_LOG_FMT("[DxFeatureCaps] Root Signature Version: 1.{}\n", static_cast<int>(version));
			break;
		}
	}

	// ===== 셰이더 모델 쿼리 =====
	caps.shaderModel = QueryHighestShaderModel(device);

	// ===== 레이트레이싱 쿼리 =====
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
	if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5))))
	{
		caps.rayTracingTier = options5.RaytracingTier;
		caps.supportsRayTracing = (caps.rayTracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED);
	}

	// ===== 메쉬 셰이더 쿼리 =====
	// ===== 샘플러 피드백 쿼리 =====
	D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
	if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7))))
	{
		caps.supportsMeshShaders = (options7.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED);
		caps.supportsSamplerFeedback = (options7.SamplerFeedbackTier != D3D12_SAMPLER_FEEDBACK_TIER_NOT_SUPPORTED);
	}


	// ===== DXGI 기능 쿼리 =====

	// Tearing 지원 확인
	ComPtr<IDXGIFactory5> factory5;
	if (SUCCEEDED(adapter->GetParent(IID_PPV_ARGS(&factory5))))
	{
		BOOL allowTearing = FALSE;
		if (SUCCEEDED(
				factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))
			))
		{
			caps.supportsTearing = (allowTearing == TRUE);
		}
	}

	// ===== 어댑터 정보 쿼리 =====
	DXGI_ADAPTER_DESC1 adapterDesc = {};
	if (SUCCEEDED(adapter->GetDesc1(&adapterDesc)))
	{
		caps.adaptorDescription = adapterDesc.Description;
		caps.vendorId = adapterDesc.VendorId;
		caps.deviceId = adapterDesc.DeviceId;
		caps.dedicatedVideoMemory = adapterDesc.DedicatedVideoMemory;
		caps.dedicatedSystemMemory = adapterDesc.DedicatedSystemMemory;
		caps.sharedSystemMemory = adapterDesc.SharedSystemMemory;
	}
	else
	{
		DEBUG_LOG_FMT("[DxFeatureCaps] Warning: Failed to get adapter description\n");
		caps.adaptorDescription = L"Unknown GPU";
	}

	return caps;
}

D3D12_FEATURE_DATA_SHADER_MODEL DxFeatureCaps::QueryHighestShaderModel(ID3D12Device* device)
{
	D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = {};

	// 가장 높은 셰이더 모델부터 순차적으로 검사
	for (auto model : SHADER_MODELS)
	{
		shaderModel.HighestShaderModel = model;

		if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel))))
		{
			DEBUG_LOG_FMT("[DxFeatureCaps] Highest Shader Model: {}\n", GetShaderModelString(model));
			return shaderModel;
		}
	}

	// 모든 검사가 실패한 경우 (매우 드문 경우)
	DEBUG_LOG_FMT("[DxFeatureCaps] Warning: No shader model supported, defaulting to 5.1\n");
	shaderModel.HighestShaderModel = D3D_SHADER_MODEL_5_1;
	return shaderModel;
}

void DxFeatureCaps::LogCapabilities() const
{
	DEBUG_LOG_FMT("=== DxFeatureCaps Information ===\n");
	LogBasicInfo();
	LogAdvancedFeatures();
	LogMemoryInfo();
	DEBUG_LOG_FMT("================================\n");
}

void DxFeatureCaps::LogBasicInfo() const
{
	DEBUG_LOG_FMT("--- Basic Information ---\n");
	std::string adapterName{adaptorDescription.begin(), adaptorDescription.end()};
	DEBUG_LOG_FMT("GPU: {}\n", adapterName);
	DEBUG_LOG_FMT("Vendor ID: 0x{:04X}\n", vendorId);
	DEBUG_LOG_FMT("Device ID: 0x{:04X}\n", deviceId);

	DEBUG_LOG_FMT("Shader Model: {}\n", GetShaderModelString(shaderModel.HighestShaderModel));
	DEBUG_LOG_FMT("Root Signature: 1.{}\n", std::to_string(static_cast<int>(rootSignature.HighestVersion)));

	DEBUG_LOG_FMT("Resource Binding Tier: {}\n", static_cast<int>(options.ResourceBindingTier));
	DEBUG_LOG_FMT("Resource Heap Tier: {}\n", static_cast<int>(options.ResourceHeapTier));
}

void DxFeatureCaps::LogAdvancedFeatures() const
{
	DEBUG_LOG_FMT("--- Advanced Features ---\n");

	DEBUG_LOG_FMT("Tearing Support: {}\n", supportsTearing ? "Yes" : "No");
	DEBUG_LOG_FMT("Ray Tracing: {} ({})\n", supportsRayTracing ? "Yes" : "No", GetRaytracingTierString());
	DEBUG_LOG_FMT("Mesh Shaders: {}\n", supportsMeshShaders ? "Yes" : "No");
	DEBUG_LOG_FMT("Sampler Feedback: {}\n", supportsSamplerFeedback ? "Yes" : "No");
}

void DxFeatureCaps::LogMemoryInfo() const
{
	DEBUG_LOG_FMT("--- Memory Information ---\n");
	DEBUG_LOG_FMT("Dedicated Video Memory: {:.1f} MB\n", dedicatedVideoMemory / (1024.0f * 1024.0f));
	DEBUG_LOG_FMT("Dedicated System Memory: {:.1f} MB\n", dedicatedSystemMemory / (1024.0f * 1024.0f));
	DEBUG_LOG_FMT("Shared System Memory: {:.1f} MB\n", sharedSystemMemory / (1024.0f * 1024.0f));
}

std::string DxFeatureCaps::GetShaderModelString(D3D_SHADER_MODEL model)
{
	switch (model)
	{
	case D3D_SHADER_MODEL_6_9:
		return "6.9";
	case D3D_SHADER_MODEL_6_8:
		return "6.8";
	case D3D_SHADER_MODEL_6_7:
		return "6.7";
	case D3D_SHADER_MODEL_6_6:
		return "6.6";
	case D3D_SHADER_MODEL_6_5:
		return "6.5";
	case D3D_SHADER_MODEL_6_4:
		return "6.4";
	case D3D_SHADER_MODEL_6_3:
		return "6.3";
	case D3D_SHADER_MODEL_6_2:
		return "6.2";
	case D3D_SHADER_MODEL_6_1:
		return "6.1";
	case D3D_SHADER_MODEL_6_0:
		return "6.0";
	case D3D_SHADER_MODEL_5_1:
		return "5.1";
	default:
		return "Unknown";
	}
}

std::string DxFeatureCaps::GetRaytracingTierString() const
{
	switch (rayTracingTier)
	{
	case D3D12_RAYTRACING_TIER_1_0:
		return "Tier 1.0";
	case D3D12_RAYTRACING_TIER_1_1:
		return "Tier 1.1";
	default:
		return "Not Supported";
	}
}