#include "stdafxClientFramework.h"
#include "DxRtPipelineState.h"
#include "DxShaderCompilerGlobal.h"
#include "DxUtils.h"

DxRtPipelineState::~DxRtPipelineState()
{
	DEBUG_LOG_FMT("[DxRtPipelineState] Destroyed\n");
}

void DxRtPipelineState::Create(ID3D12Device5* device, const std::wstring& shaderPath, uint32_t maxRecursionDepth)
{
	assert(device && "[DxRtPipelineState] Device is null");

	CreateGlobalRootSignature(device);
	CreateStateObject(device, shaderPath, maxRecursionDepth);

	DEBUG_LOG_FMT("[DxRtPipelineState] Created (MaxRecursion: {})\n", maxRecursionDepth);
}

void DxRtPipelineState::CreateGlobalRootSignature(ID3D12Device5* device)
{
	// 1. Descriptor Ranges (Table용)
	D3D12_DESCRIPTOR_RANGE1 ranges[2] = {};
	
	// Range 0: TLAS (t0)
	ranges[0] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 0};
	// Range 1: Output UAV (u0)
	ranges[1] = {D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 0};

	// 2. Root Parameters
	D3D12_ROOT_PARAMETER1 rootParams[9] = {};

	// Param 0: TLAS Table (t0)
	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[0].DescriptorTable = {1, &ranges[0]};
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// Param 1: Output UAV Table (u0)
	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[1].DescriptorTable = {1, &ranges[1]};
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// Param 2: Camera Constants (b0)
	rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParams[2].Constants = {0, 0, 16};
	rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// Param 3: Instance Buffer (t1)
	rootParams[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParams[3].Descriptor = {1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE};
	rootParams[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// Param 4: Material Buffer (t2)
	rootParams[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParams[4].Descriptor = {2, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE};
	rootParams[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// Param 5: GeoTable (t3)
	rootParams[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParams[5].Descriptor = {3, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE};
	rootParams[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// Param 6: Terrain Surface Buffer (t4)
	rootParams[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParams[6].Descriptor = {4, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE};
	rootParams[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// Param 7: Local Light Buffer (t5)
	rootParams[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParams[7].Descriptor = {5, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE};
	rootParams[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// Param 8: Local Light Constants (b1)
	rootParams[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParams[8].Constants = {1, 0, 1};
	rootParams[8].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// 3. Static Sampler (Linear Wrap)
	D3D12_STATIC_SAMPLER_DESC staticSampler = {};
	staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSampler.AddressU = staticSampler.AddressV = staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// 4. Root Signature Desc
	D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionedDesc = {};
	versionedDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	versionedDesc.Desc_1_1.NumParameters = 9;
	versionedDesc.Desc_1_1.pParameters = rootParams;
	versionedDesc.Desc_1_1.NumStaticSamplers = 1;
	versionedDesc.Desc_1_1.pStaticSamplers = &staticSampler;
	versionedDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;

	ComPtr<ID3DBlob> signature, error;
	ThrowIfFailed(D3D12SerializeVersionedRootSignature(&versionedDesc, &signature, &error));
	ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_globalRootSignature)));
	
	m_globalRootSignature->SetName(L"DXR_GlobalRootSig");
}

void DxRtPipelineState::CreateStateObject(ID3D12Device5* device, const std::wstring& shaderPath, uint32_t maxRecursionDepth)
{
	auto& compiler = GLOBAL(DxShaderCompilerGlobal);
	std::wstring shaderName = shaderPath;
	size_t lastSlash = shaderName.find_last_of(L"/\\");
	if (lastSlash != std::wstring::npos) shaderName = shaderName.substr(lastSlash + 1);
	size_t dotPos = shaderName.find_last_of(L'.');
	if (dotPos != std::wstring::npos) shaderName = shaderName.substr(0, dotPos);

	auto shaderBlob = compiler.CompileRTShader(shaderName, shaderPath, {});

	std::vector<D3D12_STATE_SUBOBJECT> subobjects;
	
	D3D12_DXIL_LIBRARY_DESC dxilLibDesc = { {shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize()}, 0, nullptr };
	subobjects.push_back({D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &dxilLibDesc});

	D3D12_HIT_GROUP_DESC hitGroupDesc = {};
	hitGroupDesc.HitGroupExport = L"HitGroup";
	hitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
	hitGroupDesc.AnyHitShaderImport = nullptr;
	hitGroupDesc.ClosestHitShaderImport = L"ClosestHitMain";
	hitGroupDesc.IntersectionShaderImport = nullptr;
	subobjects.push_back({D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &hitGroupDesc});

	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {3 * sizeof(float) + sizeof(UINT), 2 * sizeof(float)};
	subobjects.push_back({D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &shaderConfig});

	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {maxRecursionDepth};
	subobjects.push_back({D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &pipelineConfig});

	D3D12_STATE_SUBOBJECT globalRootSigObj = {D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, m_globalRootSignature.GetAddressOf()};
	subobjects.push_back(globalRootSigObj);

	D3D12_STATE_OBJECT_DESC stateObjectDesc = {D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE, (UINT)subobjects.size(), subobjects.data()};
	ThrowIfFailed(device->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(&m_stateObject)));
	ThrowIfFailed(m_stateObject.As(&m_stateObjectProps));
}

const void* DxRtPipelineState::GetRayGenShaderIdentifier() const { return m_stateObjectProps->GetShaderIdentifier(L"RayGenMain"); }
const void* DxRtPipelineState::GetMissShaderIdentifier() const { return m_stateObjectProps->GetShaderIdentifier(L"MissMain"); }
const void* DxRtPipelineState::GetHitGroupIdentifier() const { return m_stateObjectProps->GetShaderIdentifier(L"HitGroup"); }
