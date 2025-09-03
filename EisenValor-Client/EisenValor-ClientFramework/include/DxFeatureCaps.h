#pragma once
struct DxFeatureCaps
{
	static DxFeatureCaps				   Query(ID3D12Device* device, IDXGIAdapter1* adapter);
	static D3D12_FEATURE_DATA_SHADER_MODEL QueryHighestShaderModel(ID3D12Device* device);

	void LogCapabilities() const;
	void LogBasicInfo() const;
	void LogMemoryInfo() const;
	void LogAdvancedFeatures() const;

	std::string GetShaderModelString() const;
	std::string GetRaytracingTierString() const;


	// ==== General Features ====
	D3D12_FEATURE_DATA_D3D12_OPTIONS  options = {};
	D3D12_FEATURE_DATA_ROOT_SIGNATURE rootSignature = {};
	D3D12_FEATURE_DATA_SHADER_MODEL	  shaderModel = {};
	D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport = {};

	// ==== DXGI Features ====
	bool supportsTearing = false;

	// ==== Ray Tracing ====
	D3D12_RAYTRACING_TIER rayTracingTier = D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
	bool				  supportsRayTracing = false;

	// ==== Mesh Shaders ====
	bool supportsMeshShaders = false;

	// ==== Sampler Feedback ====
	bool supportsSamplerFeedback = false;

	// ==== Memory Info ====
	size_t dedicatedVideoMemory = 0;
	size_t dedicatedSystemMemory = 0;
	size_t sharedSystemMemory = 0;

	// ==== Adaptor Info ====
	std::wstring adaptorDescription;
	UINT		 vendorId = 0;
	UINT		 deviceId = 0;
};