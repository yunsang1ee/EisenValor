#include "stdafxClientFramework.h"
#include "DxRootSignatureBuilder.h"
#include "DxUtils.h"

DxRootSignatureBuilder& DxRootSignatureBuilder::AddDescriptorTable(D3D12_SHADER_VISIBILITY visibility)
{
	auto& entry = m_entries.emplace_back();
	entry.kind = RootParameterKind::DescriptorTable;
	entry.visibility = visibility;

	m_currentTableIndex = static_cast<int>(m_entries.size()) - 1;
	return *this;
}

DxRootSignatureBuilder& DxRootSignatureBuilder::AddTableRange(
	D3D12_DESCRIPTOR_RANGE_TYPE	 type,
	uint32_t					 numDescriptors,
	uint32_t					 baseRegister,
	uint32_t					 registerSpace,
	D3D12_DESCRIPTOR_RANGE_FLAGS flags,
	uint32_t					 offsetInDescriptors
)
{
	assert(m_currentTableIndex >= 0 && "[DxRootSignatureBuilder] AddDescriptorTable must be called first.");
	assert(
		m_entries[m_currentTableIndex].kind == RootParameterKind::DescriptorTable &&
		"[DxRootSignatureBuilder] Current root parameter is not a descriptor table."
	);

	DescriptorRangeEntry range = {};
	range.type = type;
	range.numDescriptors = numDescriptors;
	range.baseRegister = baseRegister;
	range.registerSpace = registerSpace;
	range.flags = flags;
	range.offsetInDescriptors = offsetInDescriptors;

	m_entries[m_currentTableIndex].ranges.push_back(range);
	return *this;
}

DxRootSignatureBuilder& DxRootSignatureBuilder::Add32BitConstants(
	uint32_t num32BitValues, uint32_t shaderRegister, uint32_t registerSpace, D3D12_SHADER_VISIBILITY visibility
)
{
	auto& entry = m_entries.emplace_back();
	entry.kind = RootParameterKind::Constants;
	entry.visibility = visibility;
	entry.constants.Num32BitValues = num32BitValues;
	entry.constants.ShaderRegister = shaderRegister;
	entry.constants.RegisterSpace = registerSpace;

	m_currentTableIndex = -1;
	return *this;
}

DxRootSignatureBuilder& DxRootSignatureBuilder::AddCBV(
	uint32_t shaderRegister, uint32_t registerSpace, D3D12_SHADER_VISIBILITY visibility
)
{
	auto& entry = m_entries.emplace_back();
	entry.kind = RootParameterKind::CBV;
	entry.visibility = visibility;
	entry.descriptor.ShaderRegister = shaderRegister;
	entry.descriptor.RegisterSpace = registerSpace;
	entry.descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;

	m_currentTableIndex = -1;
	return *this;
}

DxRootSignatureBuilder& DxRootSignatureBuilder::AddSRV(
	uint32_t shaderRegister, uint32_t registerSpace, D3D12_SHADER_VISIBILITY visibility
)
{
	auto& entry = m_entries.emplace_back();
	entry.kind = RootParameterKind::SRV;
	entry.visibility = visibility;
	entry.descriptor.ShaderRegister = shaderRegister;
	entry.descriptor.RegisterSpace = registerSpace;
	entry.descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;

	m_currentTableIndex = -1;
	return *this;
}

DxRootSignatureBuilder& DxRootSignatureBuilder::AddUAV(
	uint32_t shaderRegister, uint32_t registerSpace, D3D12_SHADER_VISIBILITY visibility
)
{
	auto& entry = m_entries.emplace_back();
	entry.kind = RootParameterKind::UAV;
	entry.visibility = visibility;
	entry.descriptor.ShaderRegister = shaderRegister;
	entry.descriptor.RegisterSpace = registerSpace;
	entry.descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;

	m_currentTableIndex = -1;
	return *this;
}

DxRootSignatureBuilder& DxRootSignatureBuilder::AddStaticSampler(
	uint32_t				   shaderRegister,
	D3D12_FILTER			   filter,
	D3D12_TEXTURE_ADDRESS_MODE addressMode,
	D3D12_SHADER_VISIBILITY	   visibility,
	uint32_t				   registerSpace
)
{
	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = filter;
	sampler.AddressU = addressMode;
	sampler.AddressV = addressMode;
	sampler.AddressW = addressMode;
	sampler.MipLODBias = 0.0f;
	sampler.MaxAnisotropy = 16;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = shaderRegister;
	sampler.RegisterSpace = registerSpace;
	sampler.ShaderVisibility = visibility;

	m_staticSamplers.push_back(sampler);
	return *this;
}

DxRootSignatureBuilder& DxRootSignatureBuilder::SetFlags(D3D12_ROOT_SIGNATURE_FLAGS flags)
{
	m_flags = flags;
	m_currentTableIndex = -1;
	return *this;
}

ComPtr<ID3D12RootSignature> DxRootSignatureBuilder::Build(ID3D12Device* device, std::string_view name) const
{
	assert(device && "[DxRootSignatureBuilder] Device is null.");

	std::vector<D3D12_ROOT_PARAMETER1> rootParameters;
	rootParameters.reserve(m_entries.size());

	size_t descriptorTableCount = 0;
	for (const auto& entry : m_entries)
	{
		if (entry.kind == RootParameterKind::DescriptorTable)
		{
			++descriptorTableCount;
		}
	}

	std::vector<std::vector<D3D12_DESCRIPTOR_RANGE1>> tableRanges;
	tableRanges.reserve(descriptorTableCount);

	for (const auto& entry : m_entries)
	{
		D3D12_ROOT_PARAMETER1 param = {};
		param.ShaderVisibility = entry.visibility;

		switch (entry.kind)
		{
		case RootParameterKind::DescriptorTable:
		{
			assert(!entry.ranges.empty() && "[DxRootSignatureBuilder] Descriptor table has no ranges.");

			auto& ranges = tableRanges.emplace_back();
			ranges.reserve(entry.ranges.size());
			for (const auto& rangeEntry : entry.ranges)
			{
				D3D12_DESCRIPTOR_RANGE1 range = {};
				range.RangeType = rangeEntry.type;
				range.NumDescriptors = rangeEntry.numDescriptors;
				range.BaseShaderRegister = rangeEntry.baseRegister;
				range.RegisterSpace = rangeEntry.registerSpace;
				range.Flags = rangeEntry.flags;
				range.OffsetInDescriptorsFromTableStart = rangeEntry.offsetInDescriptors;
				ranges.push_back(range);
			}

			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			param.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(ranges.size());
			param.DescriptorTable.pDescriptorRanges = ranges.data();
			break;
		}
		case RootParameterKind::Constants:
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
			param.Constants = entry.constants;
			break;
		case RootParameterKind::CBV:
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			param.Descriptor = entry.descriptor;
			break;
		case RootParameterKind::SRV:
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
			param.Descriptor = entry.descriptor;
			break;
		case RootParameterKind::UAV:
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
			param.Descriptor = entry.descriptor;
			break;
		default:
			assert(false && "[DxRootSignatureBuilder] Unknown root parameter kind.");
			break;
		}

		rootParameters.push_back(param);
	}

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc = {};
	rootSigDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	rootSigDesc.Desc_1_1.NumParameters = static_cast<UINT>(rootParameters.size());
	rootSigDesc.Desc_1_1.pParameters = rootParameters.data();
	rootSigDesc.Desc_1_1.NumStaticSamplers = static_cast<UINT>(m_staticSamplers.size());
	rootSigDesc.Desc_1_1.pStaticSamplers = m_staticSamplers.data();
	rootSigDesc.Desc_1_1.Flags = m_flags;

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	HRESULT			 hr = D3D12SerializeVersionedRootSignature(&rootSigDesc, &signature, &error);
	if (FAILED(hr))
	{
		if (error)
		{
			GRAPHICS_LOG_FMT(
				"[DxRootSignatureBuilder] Serialization failed: {}\n",
				static_cast<const char*>(error->GetBufferPointer())
			);
		}
		ThrowIfFailed(hr);
	}

	ComPtr<ID3D12RootSignature> rootSignature;
	ThrowIfFailed(device->CreateRootSignature(
		0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)
	));

	if (!name.empty())
	{
		DxUtils::SetDebugName(rootSignature.Get(), name);
	}

	return rootSignature;
}

void DxRootSignatureBuilder::Reset()
{
	m_entries.clear();
	m_staticSamplers.clear();
	m_flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
	m_currentTableIndex = -1;
}
