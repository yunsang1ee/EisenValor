#pragma once
#include <IComponent.h>
#include <memory>

class MaterialResource;

class PathTracingDemoLightComponent final : public ComponentBase<PathTracingDemoLightComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "PathTracingDemoLightComponent"; }
	void						 OnStart() override;
	void						 OnUpdate(float deltaTime);
	void Configure(std::shared_ptr<MaterialResource> material, const DirectX::XMFLOAT3& center, float phaseOffset);
	void CycleColor();
	void CycleOrbitSpeed();
	void ToggleAnimation() { m_animating = !m_animating; }
	bool IsAnimating() const { return m_animating; }
	const wchar_t* GetColorLabel() const;
	const wchar_t* GetComparisonHint() const;
	const wchar_t* GetOrbitSpeedLabel() const;

private:
	void							  ApplyLight();
	std::shared_ptr<MaterialResource> m_material;
	DirectX::XMFLOAT3				  m_center{};
	float							  m_orbitTime = 0.0f;
	float							  m_colorTime = 0.0f;
	float							  m_phaseOffset = 0.0f;
	uint32_t						  m_colorMode = 0;
	uint32_t						  m_orbitSpeedIndex = 1;
	bool							  m_animating = true;
};
