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

	SkinnedMeshComponent();
	~SkinnedMeshComponent() override;

	SkinnedMeshComponent(SkinnedMeshComponent&&) noexcept;
	SkinnedMeshComponent& operator=(SkinnedMeshComponent&&) noexcept;

	void OnUpdate(float dt);

	void SetSkinnedMeshResource(std::shared_ptr<SkinnedMeshResource> resource, bool loadDefaultMaterials = true);
	void SetMaterialResource(uint32_t slot, std::shared_ptr<MaterialResource> material);

	SkinnedMeshResource* GetSkinnedMeshResource() const { return m_resource.get(); }
	MaterialResource*	 GetMaterialResource(uint32_t slot) const;

	const std::vector<std::shared_ptr<MaterialResource>>& GetMaterials() const { return m_materials; }

	// 애니메이션 시스템에서 계산된 최종 행렬들을 설정
	void									SetFinalMatrices(const std::vector<DirectX::XMFLOAT4X4>& matrices);
	const std::vector<DirectX::XMFLOAT4X4>& GetFinalMatrices() const { return m_finalMatrices; }

	bool IsValid() const { return m_resource != nullptr; }

	// 프레임별 독립적인 리소스 관리 (트리플 버퍼링)
	class DxBuffer* GetSkinnedVertexBuffer(uint32_t frameIndex) const
	{
		return m_skinnedVertexBuffer[frameIndex].get();
	}
	class DxBLAS* GetBLAS(uint32_t frameIndex) const { return m_blas[frameIndex].get(); }

	void SetSkinnedVertexBuffer(uint32_t frameIndex, std::unique_ptr<class DxBuffer>&& buffer);
	void SetBLAS(uint32_t frameIndex, std::unique_ptr<class DxBLAS>&& blas);

private:
	std::shared_ptr<SkinnedMeshResource>		   m_resource;
	std::vector<std::shared_ptr<MaterialResource>> m_materials;
	std::vector<DirectX::XMFLOAT4X4>			   m_finalMatrices; // 셰이더로 전달될 최종 본 행렬

	// 애니메이션 결과물 및 가속 구조의 트리플 버퍼링
	std::unique_ptr<class DxBuffer> m_skinnedVertexBuffer[3];
	std::unique_ptr<class DxBLAS>	m_blas[3];
};
