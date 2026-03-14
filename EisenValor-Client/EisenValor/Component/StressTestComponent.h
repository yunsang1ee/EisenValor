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

#include "MovementComponent.h"
#include "Component/HealthComponent.h"
#include "Component/BattleUIControllerComponent.h"
#include "Component/TeamComponent.h"
#include "Component/VitalUIControllerComponent.h"
#include "Component/StaminaComponent.h"
#include "Component/FSM/FSMComponent.h"

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
		if (!scene)
		{
			return;
		}

		if (input.GetInputDown(VK_F4))
		{
			m_currentType =
				(m_currentType == TestType::Mesh_Sphere) ? TestType::SkinnedMesh_Human : TestType::Mesh_Sphere;
			std::string typeName = (m_currentType == TestType::Mesh_Sphere) ? "Mesh (Sphere)" : "SkinnedMesh (Human)";
			DEBUG_LOG_FMT("[StressTest] Target Type Changed to: {}\n", typeName);
		}

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
								if (auto meshRes =
										GLOBAL(ResourceGlobal).Load<MeshResource>("Resource/Models/Sphere.evmesh"))
								{
									mesh->SetMeshResource(meshRes);
								}
							}
						);
					}
					else
					{
						scene->CreateComponentWithInit<SkinnedMeshComponent>(
							obj->GetHandle(),
							[](SkinnedMeshComponent* skinned)
							{
								if (auto skinRes =
										GLOBAL(ResourceGlobal)
											.Load<SkinnedMeshResource>("Resource/Models/HumanM_Model.evskin"))
								{
									skinned->SetSkinnedMeshResource(skinRes);
								}
							}
						);

						scene->CreateComponentWithInit<MovementComponent>(
							obj->GetHandle(),
							[](MovementComponent* move)
							{
								move->SetMovementMode(MovementMode::Physics);
								move->SetMoveSpeed(5.0f);
							}
						);

						// BattleUIControllerComponent
						scene->CreateComponentWithInit<BattleUIControllerComponent>(
							obj->GetHandle(),
							[](BattleUIControllerComponent* ui)
							{
								ui->SetControlMode(BattleUIControllerComponent::ControlType::Local);
								ui->InitStance(FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL);
							}
						);

						// VitalUI (HP/Stamina/Team)
						scene->CreateComponentWithInit<HealthComponent>(
							obj->GetHandle(),
							[](HealthComponent* health)
							{
								health->SetMaxHealth(100);
								health->SetHealth(100);
							}
						);

						scene->CreateComponentWithInit<StaminaComponent>(
							obj->GetHandle(),
							[](StaminaComponent* stamina)
							{
								stamina->SetMaxStamina(100);
								stamina->SetStamina(100);
							}
						);

						scene->CreateComponentWithInit<TeamComponent>(
							obj->GetHandle(),
							[](TeamComponent* team) { team->SetTeamType(FB_ENUMS::TEAM_TYPE_OFFENSE); }
						);

						scene->CreateComponentWithInit<VitalUIControllerComponent>(
							obj->GetHandle(), [](VitalUIControllerComponent* vital) {}
						);

						// FSMComponent
						scene->CreateComponentWithInit<FSMComponent>(
							obj->GetHandle(),
							[](FSMComponent* fsm) { fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_IDLE); }
						);

						// 애니메이션 컴포넌트 추가
						scene->CreateComponentWithInit<AnimationComponent>(
							obj->GetHandle(),
							[](AnimationComponent* anim)
							{
								if (auto animRes =
										GLOBAL(ResourceGlobal)
											.Load<AnimationResource>("Resource/Animation/HumanM@Attack1H01_L.evanim"))
								{
									anim->Play(animRes, false);
								}
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
	TestType						m_currentType = TestType::Mesh_Sphere;
};
