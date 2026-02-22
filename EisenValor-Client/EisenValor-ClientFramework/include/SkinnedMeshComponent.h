#pragma once
#include "IComponent.h"
#include "SkinnedMeshResource.h"
#include <DirectXMath.h>
#include <vector>
#include <memory>

class MaterialResource;

class SkinnedMeshComponent : public ComponentBase<SkinnedMeshComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "SkinnedMeshComponent"; }

	SkinnedMeshComponent() = default;
	~SkinnedMeshComponent() override = default;

	void SetResource(std::shared_ptr<SkinnedMeshResource> resource, bool loadDefaultMaterials = true);
	void SetMaterialResource(uint32_t slot, std::shared_ptr<MaterialResource> material);

	SkinnedMeshResource* GetResource() const { return m_resource.get(); }
	MaterialResource*	 GetMaterial(uint32_t slot) const;

	const std::vector<std::shared_ptr<MaterialResource>>& GetMaterials() const { return m_materials; }

	// 애니메이션 시스템에서 계산된 최종 행렬들을 설정
	void SetFinalMatrices(const std::vector<DirectX::XMFLOAT4X4>& matrices);
	const std::vector<DirectX::XMFLOAT4X4>& GetFinalMatrices() const { return m_finalMatrices; }

	bool IsValid() const { return m_resource != nullptr; }

private:
	std::shared_ptr<SkinnedMeshResource>		   m_resource;
	std::vector<std::shared_ptr<MaterialResource>> m_materials;
	std::vector<DirectX::XMFLOAT4X4>			   m_finalMatrices; // 셰이더로 전달될 최종 본 행렬
};
