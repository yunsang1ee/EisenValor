#include "stdafxClientFramework.h"
#include "DxUtils.h"
#include <string_view>


namespace DxUtils
{

size_t GetFormatSizeInBytes(DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
		return 16;
	case DXGI_FORMAT_R32G32B32_FLOAT:
		return 12;
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
		return 8;
	case DXGI_FORMAT_R32G32_FLOAT:
		return 8;
	case DXGI_FORMAT_R8G8B8A8_UNORM:
		return 4;
	case DXGI_FORMAT_R32_FLOAT:
		return 4;
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
		return 4;
	case DXGI_FORMAT_D32_FLOAT:
		return 4;
	// 필요시 추가
	default:
		return 0;
	}
}

bool IsDepthFormat(DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_D16_UNORM:
		return true;
	default:
		return false;
	}
}

DXGI_FORMAT GetTypedFormat(DXGI_FORMAT typelessFormat, bool isDepth)
{
	switch (typelessFormat)
	{
	case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case DXGI_FORMAT_R32G32B32_TYPELESS:
		return DXGI_FORMAT_R32G32B32_FLOAT;
	case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case DXGI_FORMAT_R32G32_TYPELESS:
		return DXGI_FORMAT_R32G32_FLOAT;
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case DXGI_FORMAT_R24G8_TYPELESS:
		return isDepth ? DXGI_FORMAT_D24_UNORM_S8_UINT : DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	case DXGI_FORMAT_R32_TYPELESS:
		return isDepth ? DXGI_FORMAT_D32_FLOAT : DXGI_FORMAT_R32_FLOAT;
	case DXGI_FORMAT_R16_TYPELESS:
		return isDepth ? DXGI_FORMAT_D16_UNORM : DXGI_FORMAT_R16_FLOAT;
	default:
		return typelessFormat;
	}
}

void SetDebugName(ID3D12Object* object, const std::wstring& name)
{
	if (object)
		object->SetName(name.c_str());
}

void SetDebugName(ID3D12Object* object, std::string_view name)
{
	if (object)
	{
		std::wstring wname(name.begin(), name.end());
		object->SetName(wname.c_str());
	}
}

std::string DXGIFormatToString(DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
		return "R32G32B32A32_FLOAT";
	case DXGI_FORMAT_R32G32B32_FLOAT:
		return "R32G32B32_FLOAT";
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
		return "R16G16B16A16_FLOAT";
	case DXGI_FORMAT_R32G32_FLOAT:
		return "R32G32_FLOAT";
	case DXGI_FORMAT_R8G8B8A8_UNORM:
		return "R8G8B8A8_UNORM";
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
		return "D24_UNORM_S8_UINT";
	case DXGI_FORMAT_D32_FLOAT:
		return "D32_FLOAT";
	// 필요시 추가
	default:
		return "UNKNOWN";
	}
}

std::string ResourceStateToString(D3D12_RESOURCE_STATES state)
{
	switch (state)
	{
	case D3D12_RESOURCE_STATE_COMMON:
		return "COMMON";
	case D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER:
		return "VERTEX_AND_CONSTANT_BUFFER";
	case D3D12_RESOURCE_STATE_INDEX_BUFFER:
		return "INDEX_BUFFER";
	case D3D12_RESOURCE_STATE_RENDER_TARGET:
		return "RENDER_TARGET";
	case D3D12_RESOURCE_STATE_UNORDERED_ACCESS:
		return "UNORDERED_ACCESS";
	case D3D12_RESOURCE_STATE_DEPTH_WRITE:
		return "DEPTH_WRITE";
	case D3D12_RESOURCE_STATE_DEPTH_READ:
		return "DEPTH_READ";
	case D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:
		return "NON_PIXEL_SHADER_RESOURCE";
	case D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:
		return "PIXEL_SHADER_RESOURCE";
	case D3D12_RESOURCE_STATE_STREAM_OUT:
		return "STREAM_OUT";
	case D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT:
		return "INDIRECT_ARGUMENT";
	case D3D12_RESOURCE_STATE_COPY_DEST:
		return "COPY_DEST";
	case D3D12_RESOURCE_STATE_COPY_SOURCE:
		return "COPY_SOURCE";
	case D3D12_RESOURCE_STATE_RESOLVE_DEST:
		return "RESOLVE_DEST";
	case D3D12_RESOURCE_STATE_RESOLVE_SOURCE:
		return "RESOLVE_SOURCE";
	default:
		return "UNKNOWN";
	}
}

} // namespace DxUtils
