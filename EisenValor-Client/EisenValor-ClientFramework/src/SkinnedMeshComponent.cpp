#include "stdafxClientFramework.h"
#include "SkinnedMeshComponent.h"
#include "MaterialResource.h"
#include "ResourceGlobal.h"
#include "DxMath.h"
#include "DxBuffer.h"
#include "DxBLAS.h"

SkinnedMeshComponent::SkinnedMeshComponent() = default;
SkinnedMeshComponent::~SkinnedMeshComponent() = default;

SkinnedMeshComponent::SkinnedMeshComponent(SkinnedMeshComponent&&) noexcept = default;
SkinnedMeshComponent& SkinnedMeshComponent::operator=(SkinnedMeshComponent&&) noexcept = default;

void SkinnedMeshComponent::SetSkinnedMeshResource(
	std::shared_ptr<SkinnedMeshResource> resource, bool loadDefaultMaterials
)
{
	m_resource = std::move(resource);
	if (nullptr == m_resource)
	{
		return;
	}

	// 1. 뼈 개수에 맞춰 본 행렬 초기화 (단위행렬)
	const size_t boneCount = m_resource->GetBones().size();
	m_finalMatrices.assign(boneCount, Matrix4x4::Identity());

	// 2. 머터리얼 슬롯 설정
	uint32_t maxSlot = 0;
	for (const auto& subMesh : m_resource->GetSubMeshes())
	{
		if (subMesh.materialSlot > maxSlot)
		{
			maxSlot = subMesh.materialSlot;
		}
	}
	m_materials.assign(maxSlot + 1, GLOBAL(ResourceGlobal).GetDefaultMaterial());

	if (!loadDefaultMaterials)
	{
		return;
	}

	// 3. 기본 머터리얼 로드
	const auto& defaultGuids = m_resource->GetDefaultMaterialGuids();
	for (uint32_t i = 0; defaultGuids.size() > i && m_materials.size() > i; ++i)
	{
		auto mat = GLOBAL(ResourceGlobal).Load<MaterialResource>(defaultGuids[i]);
		if (nullptr != mat)
		{
			m_materials[i] = std::move(mat);
		}
	}
}

void SkinnedMeshComponent::SetMaterialResource(uint32_t slot, std::shared_ptr<MaterialResource> material)
{
	if (slot >= m_materials.size())
	{
		size_t oldSize = m_materials.size();
		m_materials.resize(slot + 1);

		for (size_t i = oldSize; i < slot; ++i)
		{
			m_materials[i] = GLOBAL(ResourceGlobal).GetDefaultMaterial();
		}
	}

	m_materials[slot] = std::move(material);
}

MaterialResource* SkinnedMeshComponent::GetMaterialResource(uint32_t slot) const
{
	if (slot < m_materials.size())
	{
		return m_materials[slot].get();
	}
	return nullptr;
}

void SkinnedMeshComponent::SetFinalMatrices(const std::vector<DirectX::XMFLOAT4X4>& matrices)
{
	m_finalMatrices = matrices;
}

void SkinnedMeshComponent::SetSkinnedVertexBuffer(uint32_t frameIndex, std::unique_ptr<DxBuffer>&& buffer)
{
	if (frameIndex < 3)
	{
		m_skinnedVertexBuffer[frameIndex] = std::move(buffer);
	}
}

void SkinnedMeshComponent::SetBLAS(uint32_t frameIndex, std::unique_ptr<DxBLAS>&& blas)
{
	if (frameIndex < 3)
	{
		m_blas[frameIndex] = std::move(blas);
	}
}
