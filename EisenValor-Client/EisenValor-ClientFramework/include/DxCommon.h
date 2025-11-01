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

enum class EQueueType : uint8_t
{
	Graphics = 0,
	Compute = 1,
	Copy = 2
};

struct FenceHandle
{
	EQueueType queueType;
	uint64_t   value;

	FenceHandle() : queueType(EQueueType::Graphics), value(0) {}
	FenceHandle(EQueueType qt, uint64_t v) : queueType(qt), value(v) {}
};

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