#pragma once
#include <IRenderPass.h>
#include <RenderDataSync.h>

using Microsoft::WRL::ComPtr;

class SkinningPass : public IRenderPass
{
public:
	SkinningPass() = default;
	~SkinningPass() override = default;

	void		Initialize() override;
	void		Release() override;
	void		Execute(DxFrameResource* frame, Scene* scene, RenderContext* context) override;
	void		OnResize(uint32_t width, uint32_t height) override {}
	const char* GetName() const override { return "Skinning"; }

private:
	void CreatePipeline();

private:
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pso;

	RenderDataSync<DirectX::XMFLOAT4X4> m_boneMatrixBuffer[3];

	bool m_initialized = false;
};
