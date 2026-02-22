#include "stdafxClientFramework.h"
#include "SceneResource.h"

void SceneResource::SetData(EvAsset::SceneData&& data)
{
	m_sceneData = std::move(data);
}
