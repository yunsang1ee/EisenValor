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

// MVP 행렬 추가(상수버퍼) 25.07.20
struct ConstantBuffer
{
	DirectX::XMFLOAT4X4 mvp;  // Model-View-Projection 행렬
	//XMat4x4
};


#ifndef ThrowIfFailed
#define ThrowIfFailed(x)											\
	do {															\
		HRESULT hr = (x);											\
		if (FAILED(hr)) {											\
			DEBUG_LOG_FMT("HRESULT FAILED: {} (Code: {:#x})\n",		\
			#x, static_cast<unsigned int>(hr));						\
			assert(false);											\
		}															\
	} while(false)
#endif



