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
