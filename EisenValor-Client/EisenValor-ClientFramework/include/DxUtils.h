#pragma once

namespace DxUtils
{

#pragma region Resource Barrier Utilities
// ===== Resource Barrier Utilities =====

// 리소스 전이 배리어 생성
inline D3D12_RESOURCE_BARRIER CreateTransitionBarrier(
	ID3D12Resource*		  resource,
	D3D12_RESOURCE_STATES beforeState,
	D3D12_RESOURCE_STATES afterState,
	UINT				  subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES
)
{
	return {
		.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
		.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
		.Transition =
			{.pResource = resource, .Subresource = subresource, .StateBefore = beforeState, .StateAfter = afterState}
	};
}

// UAV 배리어 생성
inline D3D12_RESOURCE_BARRIER CreateUAVBarrier(ID3D12Resource* resource)
{
	return {
		.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV,
		.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
		.UAV = {.pResource = resource}
	};
}

// 별칭 배리어 생성
inline D3D12_RESOURCE_BARRIER CreateAliasingBarrier(ID3D12Resource* before, ID3D12Resource* after)
{
	return {
		.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING,
		.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
		.Aliasing = {.pResourceBefore = before, .pResourceAfter = after}
	};
}
#pragma endregion

#pragma region Memory Alignment
// ===== Memory Alignment =====

// 상향 정렬
constexpr size_t AlignUp(size_t value, size_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

// 상수 버퍼 정렬 (256바이트)
constexpr size_t AlignConstantBuffer(size_t size)
{
	return AlignUp(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
}

// 텍스처 정렬 (512바이트)
constexpr size_t AlignTexture(size_t size)
{
	return AlignUp(size, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
}

#pragma endregion

#pragma region Descriptor Handle Utilities
// ===== Descriptor Handle Utilities =====

// CPU 핸들 오프셋 계산
inline D3D12_CPU_DESCRIPTOR_HANDLE OffsetHandle(
	D3D12_CPU_DESCRIPTOR_HANDLE baseHandle, uint32_t index, uint32_t descriptorSize
)
{
	baseHandle.ptr += static_cast<size_t>(index) * descriptorSize;
	return baseHandle;
}

// GPU 핸들 오프셋 계산
inline D3D12_GPU_DESCRIPTOR_HANDLE OffsetHandle(
	D3D12_GPU_DESCRIPTOR_HANDLE baseHandle, uint32_t index, uint32_t descriptorSize
)
{
	baseHandle.ptr += static_cast<size_t>(index) * descriptorSize;
	return baseHandle;
}

#pragma endregion

#pragma region Format Utilities
// ===== Format Utilities =====

// 포맷 크기 계산
size_t GetFormatSizeInBytes(DXGI_FORMAT format);

// 깊이 포맷 확인
bool IsDepthFormat(DXGI_FORMAT format);

// Typeless → Typed 포맷 변환
DXGI_FORMAT GetTypedFormat(DXGI_FORMAT typelessFormat, bool isDepth = false);

#pragma endregion

#pragma region Debug Utilities
// ===== Debug Utilities =====

// 디버그 이름 설정
void SetDebugName(ID3D12Object* object, const std::wstring& name);
void SetDebugName(ID3D12Object* object, std::string_view name);

// 포맷 문자열 변환 (디버그용)
std::string DXGIFormatToString(DXGI_FORMAT format);

// 리소스 상태 문자열 변환 (디버그용)
std::string ResourceStateToString(D3D12_RESOURCE_STATES state);

#pragma endregion

} // namespace DxUtils