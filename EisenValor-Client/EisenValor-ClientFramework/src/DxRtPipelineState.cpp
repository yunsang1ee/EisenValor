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
	// Global Root Signature
	// Slot 0 (Table):
	//                      TLAS (t0)
	// Slot 1 (Table):
	//                      Output UAV (u0)
	// Slot 2 (Constant):   Camera Constants (b0, inline 16 DWORD)
	// Slot 3 (Table):
	//                      Materials           (t1)
	//                      Vertex Buffer       (t2)
	//                      Index Buffer        (t3)
	//                      GeoInfo Buffer      (t4)
	//                      GeoInstBase Buffer  (t5)

	D3D12_DESCRIPTOR_RANGE1 ranges[7] = {};

	// Range 0: TLAS (t0)
	ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	ranges[0].NumDescriptors = 1;
	ranges[0].BaseShaderRegister = 0;
	ranges[0].RegisterSpace = 0;
	ranges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
	ranges[0].OffsetInDescriptorsFromTableStart = 0;

	// Range 1: Output UAV (u0)
	ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	ranges[1].NumDescriptors = 1;
	ranges[1].BaseShaderRegister = 0;
	ranges[1].RegisterSpace = 0;
	ranges[1].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
	ranges[1].OffsetInDescriptorsFromTableStart = 0;

	// clang-format off
	// Range 2: Materials (t1)
	ranges[2] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 0};
	// Range 3: Vertex Buffer (t2)
	ranges[3] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND};
	// Range 4: Index Buffer (t3)
	ranges[4] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND};
	// Range 5: GeoInfo Buffer (t4)
	ranges[5] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND};
	// Range 6: GeoInstBase Buffer (t5)
	ranges[6] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND};
	// clang-format on

	D3D12_ROOT_PARAMETER1 rootParams[4] = {};

	// Param 0: TLAS Descriptor Table
	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[0].DescriptorTable.pDescriptorRanges = &ranges[0];
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// Param 1: Output UAV Descriptor Table
	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[1].DescriptorTable.pDescriptorRanges = &ranges[1];
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// Param 2: Camera Constants (16 DWORD = 64 bytes)
	rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParams[2].Constants.ShaderRegister = 0;
	rootParams[2].Constants.RegisterSpace = 0;
	rootParams[2].Constants.Num32BitValues = 16;
	rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// Param 3: Materials Descriptor Table (t1~t5)
	rootParams[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[3].DescriptorTable.NumDescriptorRanges = 5;
	rootParams[3].DescriptorTable.pDescriptorRanges = &ranges[2];
	rootParams[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionedDesc = {};
	versionedDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	versionedDesc.Desc_1_1.NumParameters = 4;
	versionedDesc.Desc_1_1.pParameters = rootParams;
	versionedDesc.Desc_1_1.NumStaticSamplers = 0;
	versionedDesc.Desc_1_1.pStaticSamplers = nullptr;
	versionedDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;

	HRESULT hrr = D3D12SerializeVersionedRootSignature(&versionedDesc, &signature, &error);

	if (FAILED(hrr))
	{
		if (error)
		{
			const char* errorMsg = static_cast<const char*>(error->GetBufferPointer());
			DEBUG_LOG_FMT("[DxRtPipelineState] Root Signature serialization failed:\n{}\n", errorMsg);
		}
		ThrowIfFailed(hrr);
	}

	ThrowIfFailed(device->CreateRootSignature(
		0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_globalRootSignature)
	));

	m_globalRootSignature->SetName(L"DXR_GlobalRootSig");
	DEBUG_LOG_FMT("[DxRtPipelineState] Global Root Signature created\n");
}

void DxRtPipelineState::CreateStateObject(
	ID3D12Device5* device, const std::wstring& shaderPath, uint32_t maxRecursionDepth
)
{
	auto& compiler = MANAGER(DxShaderCompilerGlobal);
	auto  shaderBlob = compiler.CompileRTShader(L"RTLib", L"Resource/Shader/RaytracingLibrary.hlsl", {});

	std::vector<D3D12_STATE_SUBOBJECT> subobjects;
	subobjects.reserve(5);
	//==============================================================================
	// DXIL Library
	D3D12_DXIL_LIBRARY_DESC dxilLibDesc = {};
	dxilLibDesc.DXILLibrary.pShaderBytecode = shaderBlob->GetBufferPointer();
	dxilLibDesc.DXILLibrary.BytecodeLength = shaderBlob->GetBufferSize();
	dxilLibDesc.NumExports = 0;
	dxilLibDesc.pExports = nullptr;

	D3D12_STATE_SUBOBJECT dxilLib = {};
	dxilLib.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	dxilLib.pDesc = &dxilLibDesc;
	subobjects.push_back(dxilLib);

	// Hit Group
	const wchar_t* hitGroupName = L"HitGroup";
	const wchar_t* closestHitName = L"ClosestHitMain";

	D3D12_HIT_GROUP_DESC hitGroupDesc = {};
	hitGroupDesc.HitGroupExport = hitGroupName;
	hitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
	hitGroupDesc.ClosestHitShaderImport = closestHitName;
	hitGroupDesc.AnyHitShaderImport = nullptr;
	hitGroupDesc.IntersectionShaderImport = nullptr;

	D3D12_STATE_SUBOBJECT hitGroup = {};
	hitGroup.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	hitGroup.pDesc = &hitGroupDesc;
	subobjects.push_back(hitGroup);

	// Shader Config
	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
	shaderConfig.MaxPayloadSizeInBytes = 3 * sizeof(float) + sizeof(UINT);
	shaderConfig.MaxAttributeSizeInBytes = 2 * sizeof(float);

	D3D12_STATE_SUBOBJECT shaderConfigObj = {};
	shaderConfigObj.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	shaderConfigObj.pDesc = &shaderConfig;
	subobjects.push_back(shaderConfigObj);

	// Pipeline Config
	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
	pipelineConfig.MaxTraceRecursionDepth = maxRecursionDepth;

	D3D12_STATE_SUBOBJECT pipelineConfigObj = {};
	pipelineConfigObj.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	pipelineConfigObj.pDesc = &pipelineConfig;
	subobjects.push_back(pipelineConfigObj);

	// Global Root Signature
	D3D12_STATE_SUBOBJECT globalRootSigObj = {};
	globalRootSigObj.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	globalRootSigObj.pDesc = m_globalRootSignature.GetAddressOf();
	subobjects.push_back(globalRootSigObj);
	//==============================================================================

	D3D12_STATE_OBJECT_DESC stateObjectDesc = {};
	stateObjectDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	stateObjectDesc.NumSubobjects = static_cast<UINT>(subobjects.size());
	stateObjectDesc.pSubobjects = subobjects.data();

	ThrowIfFailed(device->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(&m_stateObject)));
	m_stateObject->SetName(L"DXR_StateObject");

	ThrowIfFailed(m_stateObject.As(&m_stateObjectProps));

	DEBUG_LOG_FMT("[DxRtPipelineState] State Object created\n");
}

const void* DxRtPipelineState::GetRayGenShaderIdentifier() const
{
	assert(m_stateObjectProps && "[DxRtPipelineState] State Object not created");
	return m_stateObjectProps->GetShaderIdentifier(L"RayGenMain");
}

const void* DxRtPipelineState::GetMissShaderIdentifier() const
{
	assert(m_stateObjectProps && "[DxRtPipelineState] State Object not created");
	return m_stateObjectProps->GetShaderIdentifier(L"MissMain");
}

const void* DxRtPipelineState::GetHitGroupIdentifier() const
{
	assert(m_stateObjectProps && "[DxRtPipelineState] State Object not created");
	return m_stateObjectProps->GetShaderIdentifier(L"HitGroup");
}