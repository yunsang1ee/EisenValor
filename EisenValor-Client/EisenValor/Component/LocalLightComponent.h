#pragma once
#include "IComponent.h"
#include <algorithm>

class LocalLightComponent : public ComponentBase<LocalLightComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "LocalLightComponent"; }

	void SetColor(const DX::XMFLOAT3& value) { m_color = value; }
	void SetIntensity(float value) { m_intensity = std::max(value, 0.0f); }
	void SetRange(float value) { m_range = std::max(value, 0.0f); }
	void SetSourceRadius(float value) { m_sourceRadius = std::max(value, 0.0f); }

	DX::XMFLOAT3 GetColor() const { return m_color; }
	float		 GetIntensity() const { return m_intensity; }
	float		 GetRange() const { return m_range; }
	float		 GetSourceRadius() const { return m_sourceRadius; }

private:
	DX::XMFLOAT3 m_color{1.0f, 0.72f, 0.38f};
	float		 m_intensity = 0.0f;
	float		 m_range = 0.0f;
	float		 m_sourceRadius = 0.0f;
};
