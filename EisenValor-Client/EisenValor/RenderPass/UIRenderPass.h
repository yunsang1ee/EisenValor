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

// 인스턴스 데이터 구조체 
struct UIInstanceData
{
	DirectX::XMFLOAT4X4 transform; 
	DirectX::XMFLOAT4	color;
	uint32_t			textureIndex;
	uint32_t			padding[3]; 					  
};

class UIRenderPass : public IRenderPass
{
public:
	UIRenderPass();
	~UIRenderPass() override;

	void		Initialize() override;
	void		Release() override;
	void		Execute(DxFrameResource* frame, Scene* scene) override;
	void		OnResize(uint32_t width, uint32_t height) override;
	const char* GetName() const override { return "UIRenderPass"; }
	// 인스턴싱 렌더링 함수
	void RenderAllUIInstanced(DxFrameResource* frame);

private:
	void CreateUIPipelineState();
	void CreateVertexBuffer();
	void CreateInstanceBuffer();

	DirectX::XMMATRIX CalculateUITransform(float x, float y, float width, float height);

	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pipelineState;
	std::unique_ptr<DxBuffer>	m_vertexBuffer;
	std::unique_ptr<DxBuffer>	m_instanceBuffer; // 인스턴스 버퍼

	uint32_t m_screenWidth = 0;
	uint32_t m_screenHeight = 0;
	uint32_t m_maxInstances = 1024; // 최대 인스턴스 개수
	bool	 m_initialized = false;
	bool	 m_vertexDataUploaded = false;
};