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

// 인스턴스 데이터 구조체 (슬롯 1) - 128바이트 정렬 (GPU 최적화)
struct UIInstanceData
{
	DirectX::XMFLOAT4X4 transform;	  // 64바이트
	DirectX::XMFLOAT4	color;		  // 16바이트
	DirectX::XMFLOAT2	uvMin;		  // 8바이트
	DirectX::XMFLOAT2	uvMax;		  // 8바이트
	uint32_t			textureIndex; // 4바이트
	uint32_t			padding[7];	  // 28바이트 (총 128바이트)
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
	DirectX::XMMATRIX CalculateUITransform(float x, float y, float width, float height, float rotationDegrees);

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
