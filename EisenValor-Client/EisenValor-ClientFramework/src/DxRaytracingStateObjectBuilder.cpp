#include "stdafxClientFramework.h"
#include "DxRaytracingStateObjectBuilder.h"
#include "DxUtils.h"

DxRaytracingStateObjectBuilder& DxRaytracingStateObjectBuilder::AddDxilLibrary(
	IDxcBlob* shaderBlob, std::span<const std::wstring> exports
)
{
	assert(shaderBlob && "[DxRaytracingStateObjectBuilder] Shader blob is null.");

	auto& entry = m_libraries.emplace_back();
	entry.blob = shaderBlob;
	entry.exportNames.assign(exports.begin(), exports.end());
	return *this;
}

DxRaytracingStateObjectBuilder& DxRaytracingStateObjectBuilder::AddHitGroup(
	std::wstring_view hitGroupName,
	std::wstring_view closestHitShader,
	std::wstring_view anyHitShader,
	std::wstring_view intersectionShader,
	D3D12_HIT_GROUP_TYPE type
)
{
	assert(!hitGroupName.empty() && "[DxRaytracingStateObjectBuilder] Hit group name is empty.");

	auto& entry = m_hitGroups.emplace_back();
	entry.name = hitGroupName;
	entry.closestHit = closestHitShader;
	entry.anyHit = anyHitShader;
	entry.intersection = intersectionShader;
	entry.type = type;
	return *this;
}

DxRaytracingStateObjectBuilder& DxRaytracingStateObjectBuilder::SetShaderConfig(
	uint32_t maxPayloadSizeInBytes, uint32_t maxAttributeSizeInBytes
)
{
	m_shaderConfig = D3D12_RAYTRACING_SHADER_CONFIG{
		maxPayloadSizeInBytes,
		maxAttributeSizeInBytes
	};
	return *this;
}

DxRaytracingStateObjectBuilder& DxRaytracingStateObjectBuilder::SetPipelineConfig(
	uint32_t maxTraceRecursionDepth
)
{
	m_pipelineConfig = D3D12_RAYTRACING_PIPELINE_CONFIG{maxTraceRecursionDepth};
	return *this;
}

DxRaytracingStateObjectBuilder& DxRaytracingStateObjectBuilder::SetGlobalRootSignature(
	ID3D12RootSignature* rootSignature
)
{
	assert(rootSignature && "[DxRaytracingStateObjectBuilder] Root signature is null.");
	m_globalRootSignature = rootSignature;
	return *this;
}

ComPtr<ID3D12StateObject> DxRaytracingStateObjectBuilder::Build(
	ID3D12Device5* device, std::string_view name
) const
{
	assert(device && "[DxRaytracingStateObjectBuilder] Device is null.");
	assert(!m_libraries.empty() && "[DxRaytracingStateObjectBuilder] No DXIL libraries.");
	assert(m_shaderConfig.has_value() && "[DxRaytracingStateObjectBuilder] Shader config is not set.");
	assert(m_pipelineConfig.has_value() && "[DxRaytracingStateObjectBuilder] Pipeline config is not set.");
	assert(m_globalRootSignature && "[DxRaytracingStateObjectBuilder] Global root signature is not set.");

	std::vector<std::vector<D3D12_EXPORT_DESC>> exportDescStorage;
	exportDescStorage.reserve(m_libraries.size());

	std::vector<D3D12_DXIL_LIBRARY_DESC> libraryDescs;
	libraryDescs.reserve(m_libraries.size());

	for (const auto& library : m_libraries)
	{
		auto& exportDescs = exportDescStorage.emplace_back();
		exportDescs.reserve(library.exportNames.size());

		for (const auto& exportName : library.exportNames)
		{
			D3D12_EXPORT_DESC exportDesc = {};
			exportDesc.Name = exportName.c_str();
			exportDesc.ExportToRename = nullptr;
			exportDesc.Flags = D3D12_EXPORT_FLAG_NONE;
			exportDescs.push_back(exportDesc);
		}

		D3D12_DXIL_LIBRARY_DESC libraryDesc = {};
		libraryDesc.DXILLibrary.pShaderBytecode = library.blob->GetBufferPointer();
		libraryDesc.DXILLibrary.BytecodeLength = library.blob->GetBufferSize();
		libraryDesc.NumExports = static_cast<UINT>(exportDescs.size());
		libraryDesc.pExports = exportDescs.empty() ? nullptr : exportDescs.data();
		libraryDescs.push_back(libraryDesc);
	}

	std::vector<D3D12_HIT_GROUP_DESC> hitGroupDescs;
	hitGroupDescs.reserve(m_hitGroups.size());

	for (const auto& hitGroup : m_hitGroups)
	{
		D3D12_HIT_GROUP_DESC hitGroupDesc = {};
		hitGroupDesc.HitGroupExport = hitGroup.name.c_str();
		hitGroupDesc.Type = hitGroup.type;
		hitGroupDesc.ClosestHitShaderImport = hitGroup.closestHit.empty() ? nullptr : hitGroup.closestHit.c_str();
		hitGroupDesc.AnyHitShaderImport = hitGroup.anyHit.empty() ? nullptr : hitGroup.anyHit.c_str();
		hitGroupDesc.IntersectionShaderImport = hitGroup.intersection.empty() ? nullptr : hitGroup.intersection.c_str();
		hitGroupDescs.push_back(hitGroupDesc);
	}

	std::vector<D3D12_STATE_SUBOBJECT> subobjects;
	subobjects.reserve(libraryDescs.size() + hitGroupDescs.size() + 3);

	for (auto& libraryDesc : libraryDescs)
	{
		subobjects.push_back({D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &libraryDesc});
	}
	for (auto& hitGroupDesc : hitGroupDescs)
	{
		subobjects.push_back({D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &hitGroupDesc});
	}

	auto shaderConfig = *m_shaderConfig;
	subobjects.push_back({D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &shaderConfig});

	auto pipelineConfig = *m_pipelineConfig;
	subobjects.push_back({D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &pipelineConfig});

	ID3D12RootSignature* globalRootSignature = m_globalRootSignature.Get();
	subobjects.push_back({D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, &globalRootSignature});

	D3D12_STATE_OBJECT_DESC stateObjectDesc = {};
	stateObjectDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	stateObjectDesc.NumSubobjects = static_cast<UINT>(subobjects.size());
	stateObjectDesc.pSubobjects = subobjects.data();

	ComPtr<ID3D12StateObject> stateObject;
	ThrowIfFailed(device->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(&stateObject)));

	if (!name.empty())
	{
		DxUtils::SetDebugName(stateObject.Get(), name);
	}

	return stateObject;
}

void DxRaytracingStateObjectBuilder::Reset()
{
	m_libraries.clear();
	m_hitGroups.clear();
	m_shaderConfig.reset();
	m_pipelineConfig.reset();
	m_globalRootSignature.Reset();
}
