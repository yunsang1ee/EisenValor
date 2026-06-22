#pragma once
#include "DxCommon.h"
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

class DxRaytracingStateObjectBuilder
{
public:
	DxRaytracingStateObjectBuilder() = default;
	~DxRaytracingStateObjectBuilder() = default;

	DxRaytracingStateObjectBuilder& AddDxilLibrary(IDxcBlob* shaderBlob, std::span<const std::wstring> exports = {});
	DxRaytracingStateObjectBuilder& AddHitGroup(
		std::wstring_view	 hitGroupName,
		std::wstring_view	 closestHitShader = {},
		std::wstring_view	 anyHitShader = {},
		std::wstring_view	 intersectionShader = {},
		D3D12_HIT_GROUP_TYPE type = D3D12_HIT_GROUP_TYPE_TRIANGLES
	);
	DxRaytracingStateObjectBuilder& SetShaderConfig(uint32_t maxPayloadSizeInBytes, uint32_t maxAttributeSizeInBytes);
	DxRaytracingStateObjectBuilder& SetPipelineConfig(uint32_t maxTraceRecursionDepth);
	DxRaytracingStateObjectBuilder& SetGlobalRootSignature(ID3D12RootSignature* rootSignature);

	[[nodiscard]] ComPtr<ID3D12StateObject> Build(ID3D12Device5* device, std::string_view name = {}) const;

	void Reset();

private:
	struct DxilLibraryEntry
	{
		ComPtr<IDxcBlob>		  blob;
		std::vector<std::wstring> exportNames;
	};

	struct HitGroupEntry
	{
		std::wstring		 name;
		std::wstring		 closestHit;
		std::wstring		 anyHit;
		std::wstring		 intersection;
		D3D12_HIT_GROUP_TYPE type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
	};

	std::vector<DxilLibraryEntry>					m_libraries;
	std::vector<HitGroupEntry>						m_hitGroups;
	std::optional<D3D12_RAYTRACING_SHADER_CONFIG>	m_shaderConfig;
	std::optional<D3D12_RAYTRACING_PIPELINE_CONFIG> m_pipelineConfig;
	ComPtr<ID3D12RootSignature>						m_globalRootSignature;
};
