#include "stdafxClient.h"
#include "SampleScene.h"

// Component
#include "Component/PlayerControllerComponent.h"
#include "Component/HealthComponent.h"
#include "Component/BattleUIControllerComponent.h"
#include "Component/TeamComponent.h"
#include "Component/VitalUIControllerComponent.h"
#include "Component/StaminaComponent.h"
#include "Component/FSM/FSMComponent.h"
#include "Component/FSM/StatePool.h"

// Engine
#include "ImageUIComponent.h"
#include "ButtonUIComponent.h"
#include "RectTransformComponent.h"

#include "Transform.h"

// Resource
#include "ResourceGlobal.h"
#include "SkinnedMeshResource.h"

#include "MeshLoader.h"

namespace
{
//clang-format off

namespace Ground
{
std::vector<Vertex> groundVertices = {
	{{-1.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 0.5f, 1.0f}},
	{{1.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 0.5f, 1.0f}},
	{{1.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 0.5f, 1.0f}},
	{{-1.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 0.5f, 1.0f}}
};
std::vector<uint32_t> groundIndices = {0, 2, 1, 0, 3, 2};
} // namespace Ground

namespace Cube
{
std::vector<Vertex> cubeVertices = { // Front face (z = 0.5)
	{{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
	{{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
	{{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
	{{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},

	// Back face (z = -0.5)
	{{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},
	{{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
	{{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.5f, 0.5f, 0.5f, 1.0f}},
	{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},

	// Left face (x = -0.5)
	{{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
	{{-0.5f, -0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.5f, 0.5f, 0.5f, 1.0f}},

	// Right face (x = 0.5)
	{{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
	{{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},
	{{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
	{{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
	 
	// Top face (y = 0.5)
	{{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},
	{{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
	{{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
	{{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 0.5f, 1.0f}},

	// Bottom face (y = -0.5)
	{{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
	{{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},
	{{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
	{{-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}}
};

std::vector<uint32_t> cubeIndices = {
	0,	1,	2,	0,	2,	3,	// Front
	4,	5,	6,	4,	6,	7,	// Back
	8,	9,	10, 8,	10, 11, // Left
	12, 13, 14, 12, 14, 15, // Right
	16, 17, 18, 16, 18, 19, // Top
	20, 21, 22, 20, 22, 23	// Bottom
};
} // namespace Cube

// clang-format on
} // namespace

void SampleScene::OnRegisterCustomComponents()
{
	RegisterComponents<
		PlayerControllerComponent, HealthComponent, BattleUIControllerComponent,
		TeamComponent, VitalUIControllerComponent, StaminaComponent, FSMComponent>();
	DEBUG_LOG_FMT("[SampleScene] Custom components registered\n");
}

void SampleScene::OnStartImpl()
{
	DEBUG_LOG_FMT("[SampleScene] OnStart called\n");

	//.evscene 로딩 함수
	// auto sceneRes = GLOBAL(ResourceGlobal).Load<SceneResource>("Resource/Scenes/MainArea.evscene");
	// if (sceneRes)
	// {
	// 	LoadFromSceneResource(sceneRes);
	// }

	CreateSceneObjects();
}

void SampleScene::CreateSceneObjects()
{
	DEBUG_LOG_FMT("[SampleScene] Creating scene objects...\n");

	/*ReserveGameObject(
		"Ground", std::nullopt,
		[this](GameObject* obj)
		{
			auto& tr = obj->GetTransform();
			tr.SetPosition(0.0f, -1.0f, 0.0f);
			tr.SetScale(20.0f);


			auto meshHandle = CreateComponentWithInit<MeshComponent>(
				obj->GetHandle(),
				[](MeshComponent* mesh) { mesh->SetMesh(Ground::groundVertices, Ground::groundIndices); }
			);
		}

	);*/

	ReserveGameObject(
		"Map", std::nullopt,
		[this](GameObject* obj)
		{
			auto& tr = obj->GetTransform();
			tr.SetPosition(0.0f, 0.0f, 0.0f); // Ground 옆
			tr.SetScale(1.0f);

			EvAsset::MeshData meshData;
			if (EvAsset::MeshLoader::LoadMeshFromObj("Resource/Models/perfect_map.obj", meshData))
			{
				DEBUG_LOG_FMT("[SampleScene] Loaded OBJ: {} vertices, {} indices\n", 
					meshData.vertices.size(), meshData.indices.size());

				std::vector<Vertex>	  clientVertices;
				std::vector<uint32_t> clientIndices = meshData.indices;

				for (const auto& v : meshData.vertices)
				{
					Vertex cv{};
					cv.position = {v.position[0], v.position[1], v.position[2]};
					cv.normal	= {v.normal[0], v.normal[1], v.normal[2]};
					cv.uv		= {v.uv0[0], v.uv0[1]};
					cv.color	= {1.0f, 1.0f, 1.0f, 1.0f};
					cv.tangent	= {v.tangent[0], v.tangent[1], v.tangent[2]};

					clientVertices.push_back(cv);
				}

				auto meshHandle = CreateComponentWithInit<MeshComponent>(
					obj->GetHandle(),
					[v = std::move(clientVertices), i = std::move(clientIndices)](MeshComponent* mesh)
					{ mesh->SetMesh(v, i, "perfect_map"); }
				);
			}
		}
	);

	/*ReserveGameObject(
		"Character", std::nullopt,
		[this](GameObject* obj)
		{
			auto& tr = obj->GetTransform();
			tr.SetPosition(10.0f, 0.0f, 0.0f);
			tr.SetScale(1.0f);

			auto res = GLOBAL(ResourceGlobal).Load<SkinnedMeshResource>("Resource/Models/HumanM_Model.evskin");
			if (res)
			{
				auto skinnedMeshHandle = CreateComponentWithInit<SkinnedMeshComponent>(
					obj->GetHandle(),
					[res](SkinnedMeshComponent* mesh)
					{ 
						mesh->SetMesh(res->GetVertices(), res->GetIndices());
						
						std::vector<SkinnedBone> clientBones;
						for (const auto& b : res->GetBones())
						{
							SkinnedBone sb{};
							sb.nameHash = b.nameHash;
							sb.parentIndex = b.parentIndex;
							sb.restPos = {b.restPos[0], b.restPos[1], b.restPos[2]};
							sb.restRot = {b.restRot[0], b.restRot[1], b.restRot[2], b.restRot[3]};
							sb.restScale = {b.restScale[0], b.restScale[1], b.restScale[2]};
							clientBones.push_back(sb);
						}
						mesh->SetSkeleton(clientBones, res->GetOffsetMatrices());
					}
				);
				DEBUG_LOG_FMT("[SampleScene] Character loaded via ResourceGlobal: {} vertices\n", res->GetVertexCount());
			}
			else
			{
				DEBUG_LOG_FMT("[SampleScene] Failed to load character via ResourceGlobal\n");
			}
		}
	);*/
	/*DX::XMFLOAT3 positions[3] = {{-4.0f, 3.0f, 0.0f}, {0.0f, 1.0f, 5.0f}, {4.0f, 3.0f, 0.0f}};

	DX::XMFLOAT3 rotations[3] = {{10.0f, 15.0f, 0.0f}, {-5.0f, 120.0f, 3.0f}, {8.0f, 210.0f, -10.0f}};

	float scales[3] = {4.0f, 1.0f, 4.0f};

	for (int i = 0; i < 3; ++i)
	{
		ReserveGameObject(
			"Cube_" + std::to_string(i), std::nullopt,
			[this, i, positions, rotations, scales](GameObject* obj)
			{
				auto& tr = obj->GetTransform();
				tr.SetPosition(positions[i].x, positions[i].y, positions[i].z);
				tr.SetRotation(rotations[i].x, rotations[i].y, rotations[i].z);
				tr.SetScale(scales[i]);
				auto meshHandle = CreateComponentWithInit<MeshComponent>(
					obj->GetHandle(), [](MeshComponent* mesh) { mesh->SetMesh(Cube::cubeVertices, Cube::cubeIndices); }
				);
			}
		);
	}*/
	DEBUG_LOG_FMT("[SampleScene] Created {} GameObjects\n", 8);
}

void SampleScene::OnEndImpl() {}