#pragma once

#include "CameraRenderData.h"
#include "DxCommon.h"
#include "Singleton.h"

#include <cstdint>

enum class StreamlineQualityMode : uint8_t
{
	DLAA,
	Quality,
	Balanced,
	Performance
};

struct StreamlineResolution
{
	uint32_t renderWidth = 0;
	uint32_t renderHeight = 0;
	uint32_t displayWidth = 0;
	uint32_t displayHeight = 0;
};

struct StreamlineEvaluateDesc
{
	ID3D12GraphicsCommandList* commandList = nullptr;
	ID3D12Fence*			   completionFence = nullptr;
	uint64_t				   completionFenceValue = 0;
	ID3D12Resource*			   colorInput = nullptr;
	ID3D12Resource*			   colorOutput = nullptr;
	ID3D12Resource*			   linearDepth = nullptr;
	ID3D12Resource*			   motionVectors = nullptr;
	ID3D12Resource*			   diffuseAlbedo = nullptr;
	ID3D12Resource*			   specularAlbedo = nullptr;
	ID3D12Resource*			   normalRoughness = nullptr;
	const CameraRenderData*	   camera = nullptr;
	uint32_t				   frameIndex = 0;
	uint32_t				   renderWidth = 0;
	uint32_t				   renderHeight = 0;
	uint32_t				   displayWidth = 0;
	uint32_t				   displayHeight = 0;
	bool					   reset = false;
};

enum class StreamlineFeatureWarmupState : uint8_t
{
	Idle = 0,
	WaitingForOptions,
	WaitingForAllocation,
	WaitingForEvaluation,
	WaitingForGpu,
	Ready,
};

struct StreamlineOptionsSignature
{
	bool				  useRayReconstruction = false;
	StreamlineQualityMode qualityMode = StreamlineQualityMode::Quality;
	uint32_t			  displayWidth = 0;
	uint32_t			  displayHeight = 0;
	bool				  valid = false;
};

class StreamlineGlobal : public Singleton<StreamlineGlobal>
{
private:
	friend class Singleton<StreamlineGlobal>;

	StreamlineGlobal() = default;
	~StreamlineGlobal() override = default;

public:
	void Initialize() override;
	void Release() override;

	void SetD3DDevice(ID3D12Device* device, IDXGIAdapter* adapter);

	[[nodiscard]] bool IsInitialized() const { return m_initialized; }
	[[nodiscard]] bool IsDLSSSupported() const { return m_dlssSupported; }
	[[nodiscard]] bool IsRayReconstructionSupported() const { return m_rayReconstructionSupported; }
	[[nodiscard]] bool IsEnabled() const { return m_enabled && m_dlssSupported; }
	[[nodiscard]] bool IsRayReconstructionEnabled() const
	{
		return IsEnabled() && m_preferRayReconstruction && m_rayReconstructionSupported;
	}

	[[nodiscard]] StreamlineResolution GetOptimalResolution(uint32_t displayWidth, uint32_t displayHeight) const;
	bool							   Evaluate(const StreamlineEvaluateDesc& desc);

	[[nodiscard]] bool						   IsFeatureWarmupActive();
	[[nodiscard]] StreamlineFeatureWarmupState GetFeatureWarmupState() const { return m_featureWarmupState; }
	[[nodiscard]] bool						   IsFeatureWarmupAllowed() const { return m_featureWarmupAllowed; }
	void SetFeatureWarmupAllowed(bool allowed) { m_featureWarmupAllowed = allowed; }
	void RequestFeatureWarmup();

	void SetEnabled(bool enabled);
	void SetPreferRayReconstruction(bool enabled);
	void SetQualityMode(StreamlineQualityMode mode);
	void RequestHistoryReset() { m_historyResetRequested = true; }

private:
	void ResetFeatureConfiguration();
	void RefreshFeatureWarmupState();

	StreamlineQualityMode		 m_qualityMode = StreamlineQualityMode::Quality;
	DirectX::XMFLOAT4X4			 m_previousViewProjection = {};
	bool						 m_initialized = false;
	bool						 m_deviceReady = false;
	bool						 m_dlssSupported = false;
	bool						 m_rayReconstructionSupported = false;
	bool						 m_enabled = true;
	bool						 m_preferRayReconstruction = true;
	bool						 m_historyResetRequested = true;
	bool						 m_hasPreviousViewProjection = false;
	bool						 m_rayReconstructionOptionsActive = false;
	bool						 m_featureWarmupAllowed = false;
	bool						 m_featureResourcesAllocated = false;
	bool						 m_allocationFailureLogged = false;
	StreamlineFeatureWarmupState m_featureWarmupState = StreamlineFeatureWarmupState::Idle;
	ComPtr<ID3D12Fence>			 m_featureWarmupFence;
	uint64_t					 m_featureWarmupFenceValue = 0;
	StreamlineOptionsSignature	 m_optionsSignature = {};
};
