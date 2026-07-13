#pragma once
#include "DxCommon.h"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

class DxRootSignatureBuilder
{
public:
	DxRootSignatureBuilder() = default;
	~DxRootSignatureBuilder() = default;

	DxRootSignatureBuilder& AddDescriptorTable(D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
	DxRootSignatureBuilder& AddTableRange(
		D3D12_DESCRIPTOR_RANGE_TYPE	 type,
		uint32_t					 numDescriptors,
		uint32_t					 baseRegister,
		uint32_t					 registerSpace = 0,
		D3D12_DESCRIPTOR_RANGE_FLAGS flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE,
		uint32_t					 offsetInDescriptors = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	);
	DxRootSignatureBuilder& Add32BitConstants(
		uint32_t				num32BitValues,
		uint32_t				shaderRegister,
		uint32_t				registerSpace = 0,
		D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL
	);
	DxRootSignatureBuilder& AddCBV(
		uint32_t				shaderRegister,
		uint32_t				registerSpace = 0,
		D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL
	);
	DxRootSignatureBuilder& AddSRV(
		uint32_t				shaderRegister,
		uint32_t				registerSpace = 0,
		D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL
	);
	DxRootSignatureBuilder& AddUAV(
		uint32_t				shaderRegister,
		uint32_t				registerSpace = 0,
		D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL
	);
	DxRootSignatureBuilder& AddStaticSampler(
		uint32_t				   shaderRegister,
		D3D12_FILTER			   filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE addressMode = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_SHADER_VISIBILITY	   visibility = D3D12_SHADER_VISIBILITY_ALL,
		uint32_t				   registerSpace = 0
	);
	DxRootSignatureBuilder& SetFlags(D3D12_ROOT_SIGNATURE_FLAGS flags);

	[[nodiscard]] ComPtr<ID3D12RootSignature> Build(ID3D12Device* device, std::string_view name = {}) const;

	void Reset();

private:
	enum class RootParameterKind : uint8_t
	{
		DescriptorTable,
		Constants,
		CBV,
		SRV,
		UAV
	};

	struct DescriptorRangeEntry
	{
		D3D12_DESCRIPTOR_RANGE_TYPE	 type = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		uint32_t					 numDescriptors = 0;
		uint32_t					 baseRegister = 0;
		uint32_t					 registerSpace = 0;
		D3D12_DESCRIPTOR_RANGE_FLAGS flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
		uint32_t					 offsetInDescriptors = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	};

	struct RootParameterEntry
	{
		RootParameterKind				  kind = RootParameterKind::DescriptorTable;
		D3D12_SHADER_VISIBILITY			  visibility = D3D12_SHADER_VISIBILITY_ALL;
		std::vector<DescriptorRangeEntry> ranges;
		D3D12_ROOT_CONSTANTS			  constants = {};
		D3D12_ROOT_DESCRIPTOR1			  descriptor = {};
	};

	std::vector<RootParameterEntry>		   m_entries;
	std::vector<D3D12_STATIC_SAMPLER_DESC> m_staticSamplers;
	D3D12_ROOT_SIGNATURE_FLAGS			   m_flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
	int									   m_currentTableIndex = -1;
};
