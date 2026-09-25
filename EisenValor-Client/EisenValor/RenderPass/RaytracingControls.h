#pragma once
#include "RenderData/RaytracingFrameRenderData.h"

class RaytracingOutputRenderData;

class RaytracingControls
{
public:
	void					  Update();
	RaytracingPath			  GetPath() const;
	void					  ApplyOutputSettings(RaytracingOutputRenderData& output) const;
	const RaytracingSettings& GetSettings() const { return m_settings; }
	void					  TogglePathTracing();
	void					  ToggleRestirPT();
	void					  ToggleDayEnvironment();

private:
	void			   TogglePhysicalRenderingBaseline();
	RaytracingSettings m_settings;
};
