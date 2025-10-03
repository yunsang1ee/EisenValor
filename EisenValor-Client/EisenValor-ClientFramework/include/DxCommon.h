#pragma once
#include "stdafxClientFramework.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <D3Dcompiler.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <DXGIDebug.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxcompiler.lib")

namespace DX = DirectX;
namespace DXP = DirectX::PackedVector;
using Microsoft::WRL::ComPtr;

class HrException : public std::runtime_error
{
public:
	HrException(HRESULT hr, const char* file, int line, const char* expression)
		: std::runtime_error(std::format(
			  "HRESULT FAILED at {}({}): {} (Code: {:#x})", file, line, expression, static_cast<unsigned int>(hr)
		  )),
		  m_hr(hr), m_file(file), m_line(line), m_expression(expression)
	{
	}


	HRESULT		GetErrorCode() const { return m_hr; }
	const char* GetFile() const { return m_file.c_str(); }
	int			GetLine() const { return m_line; }
	const char* GetExpression() const { return m_expression.c_str(); }

private:
	HRESULT		m_hr;
	std::string m_file;
	int			m_line;
	std::string m_expression;
};

//===============================================================

struct CameraConstants
{
	DirectX::XMFLOAT4X4 viewMatrix;
	DirectX::XMFLOAT4X4 projMatrix;
	DirectX::XMFLOAT4X4 viewProjInverse;
	DirectX::XMFLOAT3	cameraPosition;
	float				_padding0;
	DirectX::XMFLOAT3	cameraDirection;
	float				_padding1;
};

/// <materialMask>
/// HasAlbedoTexture    = 1 << 0,
/// HasNormalTexture    = 1 << 1,
/// HasMetallicTexture  = 1 << 2,
/// HasRoughnessTexture = 1 << 3,
/// HasEmissiveTexture  = 1 << 4,
/// IsTransparent       = 1 << 5,
/// </materialMask>
struct RTConstantBuffer
{
	DirectX::XMFLOAT3X4 mvp;		   // 12 dwords
	DWORD				materialIndex; // 1 dword
	DWORD				materialMask;  // 1 dword
	DWORD				padding[2];	   // 2 dwords (16-byte 정렬 맞춤)
}; // 총 16 dwords (64 bytes)

enum class ERTLocalRootSignatureSlot : uint32_t
{
	ConstatntSlot,
	VertexBufferSlot,
	IndexBufferSlot,
	MaterialDataSlot,
	Count
};
struct LocalRootArgument
{
	RTConstantBuffer		  cb;			// 16 dwords
	D3D12_GPU_VIRTUAL_ADDRESS vbGPUAddress; // 2 dwords
	D3D12_GPU_VIRTUAL_ADDRESS ibGPUAddress; // 2 dwords
	DWORD					  padding[4];	// 4 dwords (16-byte 정렬 맞춤)
}; // 총 24 dwords (96 bytes)

// MVP 행렬 추가(상수버퍼) 25.07.20
struct ConstantBuffer
{
	DirectX::XMFLOAT4X4 mvp; // Model-View-Projection 행렬
							 // XMat4x4
};

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                                                                               \
	do                                                                                                                 \
	{                                                                                                                  \
		HRESULT hr = (x);                                                                                              \
		if (FAILED(hr))                                                                                                \
		{                                                                                                              \
			const char* file = __FILE__;                                                                               \
			int			line = __LINE__;                                                                               \
			const char* expr = #x;                                                                                     \
                                                                                                                       \
			DEBUG_LOG_FMT(                                                                                             \
				"HRESULT FAILED: {}({}): {} (Code: {:#x})\n", file, line, expr, static_cast<unsigned int>(hr)          \
			);                                                                                                         \
			__debugbreak();                                                                                            \
			throw HrException(hr, file, line, expr);                                                                   \
		}                                                                                                              \
	} while (false)
#endif

// clang-format off
// EisenValor-ClientFramework
// 
// V - 완료, & - 현재 작업중, @ - 수정 필요, X - 보류
// 
// / Graphics /
// ├── DxCommon /
// │   ├── DxCommon.h							# d3d12.h, dxgi1_6.h, wrl, HRESULT check 등								| V
// │   ├── DxDebug.h /.cpp						# 디버그 레이어, 메시지 큐(Debug용)										| V
// │   ├── DxUtils.h /.cpp						# 공용 유틸리티 함수들													| V
// │   │ 	배리어 헬퍼, 정렬 헬퍼, 디스크립터 헬퍼, 포맷 헬퍼, 디버그 등
// │   └── DxFeatureCaps.h /.cpp				# (추가)티어·옵션 스냅샷												| &
// │
// ├── Device /
// │   ├── DxDeviceGlobal.h /.cpp				# Device + Factory + Adapter 통합 초기화								| V
// │   ├── DxCommandQueueGlobal.h /.cpp			# (Gfx / Compute / Copy 인스턴스 운영)									| V
// │   ├── DxCommandContext.h /.cpp				# ID3D12GraphicsCommandList + ID3D12CommandAllocator(한 쌍)				| V
// │   ├── DxCommandContextPool.h /.cpp			# 여러 DxCommandContext를 관리하고 순환 제공							| V
// │   └── DxSwapChain.h /.cpp					# IDXGISwapChain3, Resize, Present 등									| @
// │
// ├── Resource /
// │   ├── DxBuffer.h /.cpp						# VertexBuffer / IndexBuffer / StructuredBuffer 생성 및 SRV / UAV 생성	| 
// │   │ 	ID3D12Resource, 크기/포맷, CurrentState, 상태 추적, GPU 접근 제어
// │   ├── DxTexture.h /.cpp					# Texture2D / Texture3D / TextureCube 생성 및 SRV / UAV 생성			| 
// │   ├── DxRtvHeap.h /.cpp					# (추가)RTV 디스크립터 힙 관리											| 
// │   ├── DxDsvHeap.h /.cpp					# (추가)DSV 디스크립터 힙 관리											| 
// │   ├── DxDescriptorHeap.h /.cpp				# CBV/SRV/UAV 디스크립터 힙의 할당/해제 관리							| V
// │   ├── DxUploadHeap.h /.cpp					# UploadHeap 관리														| 
// │   │ 	선형 할당자(Linear Allocator) 또는 링 버퍼(Ring Buffer) 구조
// │   ├── DxSamplerHeap.h /.cpp				# (추가)Dynamic Sampler 전용 힙											| 
// │   ├── DxReadbackBuffer.h /.cpp				# (추가)스테이징 / 리드백(Debug용)										| X
// │   ├── DxFrameDescriptorAllocator.h /.cpp	# 프레임 단위로 Descriptor를 할당하고 관리하는 구조						| 
// │   │	Alloc###Transient, Reset으로 프레임 임시 디스크립터를 관리
// │   ├── DxGarbageCollector.h /.cpp			# (추가)펜스 기반 지연 파괴												| 
// │   └── DxResourceStateTracker.h /.cpp		# (추가)리소스 상태 추적 및 배리어 관리									|
// │
// ├── Pipeline /
// │   ├── DxShaderCompiler.h /.cpp				# (DXC + 디스크 캐시 권장)												| 
// │   ├── DxRootSignature.h /.cpp				# Root Signature 생성과 셰이더 바인딩, Static Sampler					| 
// │   │ 	HLSL에서 직접 루트 시그니처를 정의(Root Signature 1.1)
// │   └── DxPipelineState.h /.cpp				# (PSO 캐시 키 / Serialize)												| 
// │		상태(Blend, Depth-Stencil 등)를 기반으로 해시 키(Hash Key)를 생성하고 std::unordered_map 같은 곳에 캐싱
// │
// ├── Raytracing /								# (추가)DXR 모듈
// │   ├── DxBLAS.h /.cpp						# BLAS 생성 및 업데이트													| 
// │   ├── DxTLAS.h /.cpp						# TLAS 생성 및 업데이트													| 
// │   ├── DxRtShaderTable.h /.cpp				# SBT 작성(Identifier/LocalRootData/정렬)								| 
// │   ├── DxRtPipelineState.h /.cpp			# RTPSO 생성 및 캐싱 (서브오브젝트 스트림)								| 
// │
// └── Renderer /
//     ├── DxRendererGlobal.h /.cpp				# 전체 렌더링 파이프라인												| 
//     ├── DxFrameResource.h /.cpp				# 프레임 단위로 CommandContextPool과 연동								| 
//     ├── RenderPass.h							# 렌더 패스 인터페이스(필요시 핑퐁 버퍼까지 관리)						|
//     ├── DxPostProcessChain.h /.cpp			# (추가)포스트 프로세싱 체인 관리(추후 삭제)							| 
//     └── RenderGraph.h /.cpp					# (추가)경량 그래프														| X
// 
//
// # Render
// RenderPass = “한 작업(렌더/컴퓨트/RT) 단위”의 입·출력·상태 요구·실행을 캡슐화한 클래스
// RenderGraph = 패스들의 순서/전이/자원 생명주기/별칭/임시 디스크립터를 중앙에서 자동 관리
// 
// # DescriptorHeap
// CBV/SRV/UAV 디스크립터 힙은 DxDescriptorHeap으로 관리(Bindless Descriptor Heap)
// 힙에 만든 디스크립터를 DescriptorTable로 노출하고,
// 셰이더에서 Texture2D t[] : register(t0, space0) 식으로 인덱싱
// 
// RTV/DSV 디스크립터 힙은 별도로 관리(CPU-Only)
//  
// 
// Dynamic Sampler - DxSamplerHeap으로 관리(Bindless Descriptor Heap)
// Static Sampler - DxRootSignature에서 정의
// 
// 
// # Resource Barrier
// Transition Barrier
// Subresource: 리소스의 특정 부분만 전환할 때 사용
// 
// UAV Barrier
// 이전에 요청된 모든 UAV 작업이 끝나기 전까지는, 다음에 오는 UAV 작업을 시작하지 마
// 
// 
// # Resource Manage
// 프레임 한정 뷰(포스트 체인 중간물)는 DxFrameDescriptorAllocator로 매 프레임 재사용/초기화
// 리소스에서 뷰 호환성을 늘 보장 강제
// DxDescriptorHeap은 뷰, DxBuffer, DxTexture는 리소스와 메인 뷰 인덱스
// struct DxSrvHandle { D3D12_CPU_DESCRIPTOR_HANDLE cpu; D3D12_GPU_DESCRIPTOR_HANDLE gpu; uint32_t index; };
// 
// # 생명주기/GC 규약
// 뷰가 바인딩된 드로우/디스패치가 끝나기 전 절대 해제 금지
// 영구 뷰: 리소스 파괴 시 GC 큐에 (fence 값과 함께) 등록해 안전 시점에 해제
// 일시 뷰: 프레임 끝에 통째로 리셋