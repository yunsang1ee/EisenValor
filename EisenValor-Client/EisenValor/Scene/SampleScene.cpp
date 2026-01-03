#include "stdafxClient.h"
#include "SampleScene.h"

namespace Resources
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
} // namespace Resources

void SampleScene::OnRegisterCustomComponents()
{
	// RegisterComponents<
	//		MyCustomComponent1,
	//	>();
	DEBUG_LOG_FMT("[SampleScene] Custom components registered\n");
}

void SampleScene::OnStartImpl()
{
	DEBUG_LOG_FMT("[SampleScene] OnStart called\n");
	CreateSceneObjects();
}

void SampleScene::CreateSceneObjects()
{
	DEBUG_LOG_FMT("[SampleScene] Creating scene objects...\n");
	{
		CreateGameObject(
			"LocalPlayer", std::nullopt,
			[this](GameObject* obj)
			{
				CreateComponent<MovementComponent>(obj->GetHandle());

				CreateComponentWithInit<MeshComponent>(
					obj->GetHandle(), [](MeshComponent* mesh)
					{ mesh->SetMesh(Resources::Cube::cubeVertices, Resources::Cube::cubeIndices); }
				);

				CreateComponentWithInit<CameraComponent>(
					obj->GetHandle(), [](CameraComponent* cam) { cam->SetAsMainCamera(); }
				);
			}
		);
	}

	{
		CreateGameObject(
			"Ground", std::nullopt,
			[this](GameObject* obj)
			{
				auto& tr = obj->GetTransform();
				tr.SetPosition(0.0f, 0.0f, 0.0f);
				tr.SetScale(10.0f);


				auto meshHandle = CreateComponentWithInit<MeshComponent>(
					obj->GetHandle(), [](MeshComponent* mesh)
					{ mesh->SetMesh(Resources::Ground::groundVertices, Resources::Ground::groundIndices); }
				);
			}
		);
	}

	DX::XMFLOAT3 positions[3] = {{-4.0f, 3.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {4.0f, 3.0f, 0.0f}};

	DX::XMFLOAT3 rotations[3] = {{10.0f, 15.0f, 0.0f}, {-5.0f, 120.0f, 3.0f}, {8.0f, 210.0f, -10.0f}};

	float scales[3] = {4.0f, 1.0f, 4.0f};

	for (int i = 0; i < 3; ++i)
	{
		CreateGameObject(
			"Cube_" + std::to_string(i), std::nullopt,
			[this, i, positions, rotations, scales](GameObject* obj)
			{
				auto& tr = obj->GetTransform();
				tr.SetPosition(positions[i].x, positions[i].y, positions[i].z);
				tr.SetRotation(rotations[i].x, rotations[i].y, rotations[i].z);
				tr.SetScale(scales[i]);
				auto meshHandle = CreateComponentWithInit<MeshComponent>(
					obj->GetHandle(), [](MeshComponent* mesh)
					{ mesh->SetMesh(Resources::Cube::cubeVertices, Resources::Cube::cubeIndices); }
				);
			}
		);
	}

	DEBUG_LOG_FMT("[SampleScene] Created {} GameObjects\n", 4);
}

void SampleScene::OnEndImpl() {}
