#pragma once
#include "IComponent.h"
#include "InputGlobal.h"
#include "SceneGlobal.h"
#include "Scene.h"
#include "MeshComponent.h"
#include "SkinnedMeshComponent.h"
#include "ResourceGlobal.h"
#include "MeshResource.h"
#include "SkinnedMeshResource.h"
#include "Transform.h"
#include <vector>
#include <string>

class StressTestComponent : public ComponentBase<StressTestComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "StressTestComponent"; }

	enum class TestType
	{
		Mesh_Sphere,
		SkinnedMesh_Human
	};

	void OnUpdate(float delta)
	{
		auto& input = GLOBAL(InputGlobal);
		auto* scene = GLOBAL(SceneGlobal).GetActiveScene();
		if (!scene) return;

		// F4: 생성 대상 변경 (Sphere <-> Human)
		if (input.GetInputDown(VK_F4))
		{
			m_currentType = (m_currentType == TestType::Mesh_Sphere) ? TestType::SkinnedMesh_Human : TestType::Mesh_Sphere;
			std::string typeName = (m_currentType == TestType::Mesh_Sphere) ? "Mesh (Sphere)" : "SkinnedMesh (Human)";
			DEBUG_LOG_FMT("[StressTest] Target Type Changed to: {}\n", typeName);
		}

		// F2: 오브젝트 하나 생성
		if (input.GetInputDown(VK_F2))
		{
			float x = (rand() % 200 - 100) * 0.1f;
			float z = (rand() % 200 - 100) * 0.1f;
			
			std::string objName = (m_currentType == TestType::Mesh_Sphere) ? "Test_Sphere" : "Test_Human";

			auto handle = scene->ReserveGameObject(
				objName, std::nullopt,
				[scene, x, z, type = m_currentType](GameObject* obj)
				{
					obj->GetTransform().SetPosition(x, 2.0f, z);
					
					if (type == TestType::Mesh_Sphere)
					{
						scene->CreateComponentWithInit<MeshComponent>(
							obj->GetHandle(),
							[](MeshComponent* mesh)
							{
								auto meshRes = GLOBAL(ResourceGlobal).Load<MeshResource>("Resource/Models/Sphere.evmesh");
								if (meshRes) mesh->SetMeshResource(meshRes);
							}
						);
					}
					else
					{
						scene->CreateComponentWithInit<SkinnedMeshComponent>(
							obj->GetHandle(),
							[](SkinnedMeshComponent* skinned)
							{
								auto skinRes = GLOBAL(ResourceGlobal).Load<SkinnedMeshResource>("Resource/Models/HumanM_Model.evskin");
								if (skinRes) skinned->SetSkinnedMeshResource(skinRes);
							}
						);
					}
				}
			);
			
			m_spawnedObjects.push_back(handle);
			DEBUG_LOG_FMT("[StressTest] Spawned: {} (Total: {})\n", objName, m_spawnedObjects.size());
		}

		// F3: 생성된 오브젝트 하나씩 삭제 (최근 생성순)
		if (input.GetInputDown(VK_F3))
		{
			if (!m_spawnedObjects.empty())
			{
				auto handle = m_spawnedObjects.back();
				scene->DestroyGameObject(handle);
				m_spawnedObjects.pop_back();
				DEBUG_LOG_FMT("[StressTest] Destroyed one object (Remaining: {})\n", m_spawnedObjects.size());
			}
		}
	}

private:
	std::vector<GameObject::Handle> m_spawnedObjects;
	TestType m_currentType = TestType::Mesh_Sphere;
};
