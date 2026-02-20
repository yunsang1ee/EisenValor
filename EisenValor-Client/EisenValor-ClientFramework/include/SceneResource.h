#pragma once
#include "IResource.h"
#include "SceneData.h"
#include <vector>

class SceneResource : public ResourceBase<SceneResource>
{
public:
	SceneResource() = default;
	virtual ~SceneResource() override = default;

	void SetData(EvAsset::SceneData&& data);

	const std::vector<EvAsset::Node>&			  GetNodes() const { return m_sceneData.nodes; }
	const std::vector<EvAsset::SceneData::ComponentEntry>& GetComponents() const { return m_sceneData.components; }

private:
	EvAsset::SceneData m_sceneData;
};
