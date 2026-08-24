#pragma once

#include "DxTexture.h"
#include "RenderDataPolicy.h"

#include <cstdint>
#include <memory>

enum class DlssOutputStatus : uint8_t
{
	NotRun = 0,
	Disabled,
	MissingInput,
	MissingGuideInput,
	EvaluateFailed,
	Valid,
	WarmingUp,
};

namespace DlssMissingInputMask
{
inline constexpr uint32_t None = 0u;
inline constexpr uint32_t NoRaytracingOutput = 1u << 0u;
inline constexpr uint32_t NoCandidateData = 1u << 1u;
inline constexpr uint32_t NoCameraData = 1u << 2u;
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
inline constexpr uint32_t BypassRequested = 1u << 3u;
#endif
inline constexpr uint32_t CandidateInvalid = 1u << 4u;
inline constexpr uint32_t NoColorInput = 1u << 5u;
inline constexpr uint32_t NoColorOutput = 1u << 6u;
inline constexpr uint32_t NoDepth = 1u << 7u;
inline constexpr uint32_t NoMotionVectors = 1u << 8u;
inline constexpr uint32_t NoDiffuseAlbedo = 1u << 9u;
inline constexpr uint32_t NoSpecularAlbedo = 1u << 10u;
inline constexpr uint32_t NoNormalRoughness = 1u << 11u;
} // namespace DlssMissingInputMask

class DlssOutputRenderData : public RenderDataBase<DlssOutputRenderData>
{
public:
	void Release() override
	{
		outputTexture.reset();
		validThisFrame = false;
		usedRayReconstruction = false;
		status = DlssOutputStatus::NotRun;
		missingInputMask = DlssMissingInputMask::None;
	}

	std::shared_ptr<DxTexture> outputTexture;
	bool validThisFrame = false;
	bool usedRayReconstruction = false;
	DlssOutputStatus status = DlssOutputStatus::NotRun;
	uint32_t missingInputMask = DlssMissingInputMask::None;
};
