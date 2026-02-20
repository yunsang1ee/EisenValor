#pragma once
#include <IRenderPass.h>
#include <DxBuffer.h>
#include <DxShaderCompilerGlobal.h>
#include <DxDeviceGlobal.h>
#include <DxFrameResource.h>
#include <DxSwapChain.h>

struct UIVertex
{
	DirectX::XMFLOAT2 position;
	DirectX::XMFLOAT4 color;
};

// 인스턴스 데이터 구조체 (슬롯 1)
struct UIInstanceData
{
	DirectX::XMFLOAT4X4 transform;
	DirectX::XMFLOAT4	color;
	DirectX::XMFLOAT2	uvMin;		  // UV 좌상단
	DirectX::XMFLOAT2	uvMax;		  // UV 우하단
	uint32_t			textureIndex; // Bindless 텍스처 인덱스
	uint32_t			padding[3];	  
};

class UIRenderPass : public IRenderPass
{
public:
	UIRenderPass();
	~UIRenderPass() override;

	virtual void		Initialize() override;
	virtual void		Release() override;
	virtual void		Execute(DxFrameResource* frame, Scene* scene, RenderContext* renderContext) override;
	virtual void		OnResize(uint32_t width, uint32_t height) override;
	virtual const char* GetName() const override { return "UIRenderPass"; }

	// UI 렌더링 로직
	void RenderAllUIInstanced(DxFrameResource* frame, Scene* scene);

private:
	// 파이프라인 및 리소스 생성
	void CreateUIPipelineState();
	void CreateVertexBuffer();
	void CreateInstanceBuffer();

	// 화면 좌표 -> NDC 행렬 계산
	DirectX::XMMATRIX CalculateUITransform(float x, float y, float width, float height);

private:
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pipelineState;

	std::unique_ptr<DxBuffer> m_vertexBuffer;
	std::unique_ptr<DxBuffer> m_instanceBuffer;

	uint32_t m_screenWidth = 0;
	uint32_t m_screenHeight = 0;
	uint32_t m_maxInstances = 1024;

	bool m_initialized = false;
	bool m_vertexDataUploaded = false;
};
