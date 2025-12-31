#include "stdafxClientFramework.h"
#include "MeshComponent.h"
#include "GameObject.h"
#include "Scene.h"
#include "SceneGlobal.h"

void MeshComponent::SetMesh(
	const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, std::string_view name
)
{
	m_vertices = vertices;
	m_indices = indices;

	auto* scene = GLOBAL(SceneGlobal).GetActiveScene();
	if (scene)
	{
		auto* myGameObject = scene->TryGetGameObject(GetOwner());
		if (myGameObject)
		{
			this->m_name = name.empty() ? myGameObject->GetName() + "_Mesh" : std::string(name);
		}
		else
		{
			this->m_name = name.empty() ? "Mesh" : std::string(name);
		}
	}
	else
	{
		this->m_name = name.empty() ? "Mesh" : std::string(name);
	}
}