#include "stdafxClientFramework.h"
#include "MeshComponent.h"
#include "MeshResource.h"
#include "MaterialResource.h"
#include "ResourceGlobal.h"

void MeshComponent::SetMeshResource(std::shared_ptr<MeshResource> mesh, bool loadDefaultMaterials)
{
	m_meshResource = std::move(mesh);

	if (nullptr == m_meshResource)
	{
		return;
	}

	uint32_t maxSlot = 0;
	for (const auto& subMesh : m_meshResource->GetSubMeshes())
	{
		if (subMesh.materialSlot > maxSlot)
		{
			maxSlot = subMesh.materialSlot;
		}
	}
	m_materials.assign(maxSlot + 1, nullptr);

	if (!loadDefaultMaterials)
	{
		return;
	}

	const auto& defaultGuids = m_meshResource->GetDefaultMaterialGuids();
	for (uint32_t i = 0; defaultGuids.size() > i && m_materials.size() > i; ++i)
	{
		auto mat = GLOBAL(ResourceGlobal).Load<MaterialResource>(defaultGuids[i]);
		if (nullptr != mat)
		{
			m_materials[i] = std::move(mat);
		}
	}
}

void MeshComponent::SetMaterialResource(uint32_t slot, std::shared_ptr<MaterialResource> material)
{
	if (slot < m_materials.size())
	{
		m_materials[slot] = std::move(material);
	}
	else
	{
		m_materials.resize(slot + 1);
		m_materials[slot] = std::move(material);
	}
}

MaterialResource* MeshComponent::GetMaterial(uint32_t slot) const
{
	if (slot < m_materials.size())
	{
		return m_materials[slot].get();
	}
	return nullptr;
}