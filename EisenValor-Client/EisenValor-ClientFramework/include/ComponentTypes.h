#pragma once
#include <cstdint>

// 자주 쓰는 핵심 컴포넌트만 Enum으로 관리
// 나머지는 vector로 관리
enum class CoreComponentType : uint8_t
{
	MeshRenderer = 0, // 렌더링 컴포넌트
	Camera,			  // 카메라 컴포넌트
	Movement,		  // 이동/입력 처리 컴포넌트
	END				  // 배열 크기용
};

constexpr size_t CORE_COMPONENT_COUNT = static_cast<size_t>(CoreComponentType::END);