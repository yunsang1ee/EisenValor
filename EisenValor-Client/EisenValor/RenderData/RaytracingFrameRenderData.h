#pragma once
#include <RenderDataPolicy.h>
#include <DirectXMath.h>
#include <cstdint>
#include "RaytracingCommon.h"

enum class RaytracingPath : uint8_t
{
	Realtime,
	Reference,
	RestirCandidate
};

struct RaytracingSettings
{
	bool	 usePathTracing = false;
	bool	 useRestirPT = true;
	bool	 usePhysicalRenderingBaseline = false;
	bool	 useDayEnvironment = true;
	uint32_t restirCandidateMask = RESTIR_CANDIDATE_ALL;
	uint32_t restirPrimarySpp = 1u;
	uint32_t restirEmissiveProfileStage = RESTIR_EMISSIVE_PROFILE_FULL;
	uint64_t restirHistoryGeneration = 1;
	uint64_t profileRevision = 0;
};


class RaytracingFrameRenderData : public RenderDataBase<RaytracingFrameRenderData>
{
public:
	void Release() override
	{
		ready = false;
		dispatched = false;
		hasCamera = false;
		hasPreviousFrame = false;
	}
	bool IsValid() const override { return ready; }

	RaytracingPath		path = RaytracingPath::RestirCandidate;
	RaytracingSettings	settings;
	DirectX::XMFLOAT4X4 viewProjection = {};
	DirectX::XMFLOAT4X4 previousViewProjection = {};
	uint64_t			historySignature = 0;
	uint32_t			frameIndex = 0;
	uint32_t			frameSeed = 0;
	uint32_t			animatedBlasCount = 0;
	uint32_t			instanceCount = 0;
	uint32_t			staticInstanceCount = 0;
	bool				hasCamera = false;
	bool				hasPreviousFrame = false;
	bool				ready = false;
	bool				dispatched = false;
};
