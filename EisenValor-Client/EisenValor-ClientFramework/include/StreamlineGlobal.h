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
	ID3D12Resource* colorInput = nullptr;
	ID3D12Resource* colorOutput = nullptr;
	ID3D12Resource* linearDepth = nullptr;
	ID3D12Resource* motionVectors = nullptr;
	ID3D12Resource* diffuseAlbedo = nullptr;
	ID3D12Resource* specularAlbedo = nullptr;
	ID3D12Resource* normalRoughness = nullptr;
	const CameraRenderData* camera = nullptr;
	uint32_t frameIndex = 0;
	uint32_t renderWidth = 0;
	uint32_t renderHeight = 0;
	uint32_t displayWidth = 0;
	uint32_t displayHeight = 0;
	bool reset = false;
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
	bool Evaluate(const StreamlineEvaluateDesc& desc);

	void SetEnabled(bool enabled);
	void SetPreferRayReconstruction(bool enabled);
	void SetQualityMode(StreamlineQualityMode mode);
	void RequestHistoryReset() { m_historyResetRequested = true; }

private:
	StreamlineQualityMode m_qualityMode = StreamlineQualityMode::Quality;
	DirectX::XMFLOAT4X4 m_previousViewProjection = {};
	bool m_initialized = false;
	bool m_deviceReady = false;
	bool m_dlssSupported = false;
	bool m_rayReconstructionSupported = false;
	bool m_enabled = true;
	bool m_preferRayReconstruction = true;
	bool m_historyResetRequested = true;
	bool m_hasPreviousViewProjection = false;
};
