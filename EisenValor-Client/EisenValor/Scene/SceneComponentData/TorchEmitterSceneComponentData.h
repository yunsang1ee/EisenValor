#pragma once
#include "AssetFormat.h"
#include <cstring>
#include <vector>

class TorchEmitterSceneComponentData
{
public:
	static constexpr uint64_t StaticTypeId() { return EvAsset::HashString("TorchEmitterMetadata"); }

	bool Deserialize(uint32_t version, const std::vector<std::byte>& data)
	{
		if (version != 1 || data.size() != sizeof(Payload))
		{
			return false;
		}

		std::memcpy(&m_payload, data.data(), sizeof(Payload));
		return true;
	}

	float		 GetIntensity() const { return m_payload.intensity; }
	float		 GetRange() const { return m_payload.range; }
	float		 GetSourceRadius() const { return m_payload.sourceRadius; }
	const float* GetColor() const { return m_payload.color; }
	float		 GetFlickerAmplitude() const { return m_payload.flickerAmplitude; }
	float		 GetFlickerFrequency() const { return m_payload.flickerFrequency; }

private:
	struct Payload
	{
		float color[3];
		float intensity;
		float range;
		float sourceRadius;
		float flickerAmplitude;
		float flickerFrequency;
	};

	Payload m_payload{};
};
