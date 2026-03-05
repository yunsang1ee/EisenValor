#pragma once
#include "IComponent.h"
#include <memory>

class MeshResource;
class MaterialResource;

class MeshComponent : public ComponentBase<MeshComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "MeshComponent"; }

	MeshComponent() = default;
	~MeshComponent() override = default;

	void SetMeshResource(std::shared_ptr<MeshResource> meshRes, bool loadDefaultMaterials = true);
	void SetMaterialResource(uint32_t slot, std::shared_ptr<MaterialResource> material);

	MeshResource*	  GetMeshResource() const { return m_meshResource.get(); }
	MaterialResource* GetMaterial(uint32_t slot) const;

	const std::vector<std::shared_ptr<MaterialResource>>& GetMaterials() const { return m_materials; }

	bool IsValid() const { return nullptr != m_meshResource; }

private:
	std::shared_ptr<MeshResource>				   m_meshResource;
	std::vector<std::shared_ptr<MaterialResource>> m_materials;
};