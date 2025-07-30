#pragma once
#include "stdafxClientFramework.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <DXGIDebug.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#pragma comment(lib, "d3d12.lib")

namespace DX = DirectX;
namespace DXP = DirectX::PackedVector;
using Microsoft::WRL::ComPtr;

class HrException : public std::runtime_error
{
public:
	HrException(HRESULT hr, const char* file, int line, const char* expression)
		: std::runtime_error(std::format("HRESULT FAILED at {}({}): {} (Code: {:#x})",
			file, line, expression, static_cast<unsigned int>(hr))
		),
		m_hr(hr),
		m_file(file),
		m_line(line),
		m_expression(expression)
	{
	}

	HRESULT GetErrorCode() const { return m_hr; }
	const char* GetFile() const { return m_file.c_str(); }
	int GetLine() const { return m_line; }
	const char* GetExpression() const { return m_expression.c_str(); }

private:
	HRESULT m_hr;
	std::string m_file;
	int m_line;
	std::string m_expression;
};

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)											        \
	do {															        \
		HRESULT hr = (x);											        \
		if (FAILED(hr)) {											        \
			const char* file = __FILE__;                                    \
			int line = __LINE__;                                            \
			const char* expr = #x;                                          \
																			\
			DEBUG_LOG_FMT("HRESULT FAILED: {}({}): {} (Code: {:#x})\n",     \
						  file, line, expr, static_cast<unsigned int>(hr));	\
			__debugbreak();													\
			throw HrException(hr, file, line, expr);                        \
		}                                                                   \
	} while(false)
#endif


// /Graphics/
// ├── DxCommon /
// │   ├── DxCommon.h						# d3d12.h, dxgi1_6.h, wrl, HRESULT check 등
// │   ├── DxDebug.h / .cpp					# 디버그 레이어, 메시지 큐
// │   └── DxUtils.h / .cpp					# Barrier, LoadShader, CreateBuffer 등
// ├── Device /
// │   ├── DxDevice.h / .cpp				# Device + Factory + Adapter 통합 초기화
// │   ├── DxCommandQueue.h / .cpp			# ID3D12CommandQueue만 관리
// │   ├── DxCommandContext.h / .cpp		# ID3D12GraphicsCommandList + ID3D12CommandAllocator(한 쌍)
// │   ├── DxCommandContextPool.h / .cpp	# 여러 DxCommandContext를 관리하고 순환 제공
// │   └── DxSwapChain.h / .cpp				# IDXGISwapChain3, Resize, Present 등
// ├── Resource /
// │   ├── DxBuffer.h / .cpp				# Vertex / Index / Constant buffer
// │   ├── DxTexture.h / .cpp				# Texture, SRV 생성
// │   ├── DxUploadHeap.h / .cpp			# UploadHeap 관리
// │   └── DxDescriptorHeap.h / .cpp		# RTV / DSV / CBV - SRV - UAV 힙
// │   └── DxFrameDescriptorAllocator.h / .cpp # 프레임 단위로 Descriptor를 할당하고 관리하는 구조
// ├── Pipeline /
// │   ├── DxShaderCompiler.h / .cpp		# Dxc / D3DCompile
// │   ├── DxRootSignature.h / .cpp			# ID3D12RootSignature
// │   └── DxPipelineState.h / .cpp			# PSO 구성, 캐싱 구조
// ├── Renderer /
// │   ├── DxRenderer.h / .cpp				# IRenderer 인터페이스 구현
// │   ├── DxFrameResource.h / .cpp			# CommandAllocator / Fence 대신 CommandContextPool과 연동
// │   └── RenderPass_Triangle.h / .cpp		# 샘플 렌더패스

// Graphics/
// ├── DxCommon/
// │   ├── DxCommon.h						|	V
// │   ├── DxDebug.h/.cpp					|	V
// │   └── DxUtils.h/.cpp					|	
// ├── Device/
// │   ├── DxDevice.h/.cpp					|	V
// │   ├── DxCommandQueue.h / .cpp			|	V
// │   ├── DxCommandContext.h / .cpp		|	V
// │   ├── DxCommandContextPool.h / .cpp	|	V
// │   └── DxSwapChain.h/.cpp				|	V
// ├── Resource/
// │   ├── DxBuffer.h/.cpp					|	
// │   ├── DxTexture.h/.cpp					|	
// │   ├── DxUploadHeap.h/.cpp				|	
// │   └── DxDescriptorHeap.h/.cpp			|	V
// │   └── DxFrameDescriptorAllocator.h/.cpp|	@
// ├── Pipeline/ 
// │   ├── DxShaderCompiler.h/.cpp			|	
// │   ├── DxRootSignature.h/.cpp			|	
// │   └── DxPipelineState.h/.cpp			|	
// ├── Renderer/
// │   ├── DxRenderer.h/.cpp				|	
// │   ├── DxFrameResource.h/.cpp			|	
// │   └── RenderPass_Triangle.h/.cpp		|	
// 