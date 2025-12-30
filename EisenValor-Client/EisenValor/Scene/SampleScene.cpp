#include "stdafxClient.h"
#include "SampleScene.h"

void SampleScene::OnRegisterCustomComponents() 
{
	//RegisterComponents<
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
		auto  groundHandle = CreateGameObject("Ground");
		auto* ground = TryGetGameObject(groundHandle);
		if (ground)
		{
			auto& tr = ground->GetTransform();
			tr.SetPosition(0.0f, 0.0f, 0.0f);
			tr.SetScale(10.0f);

			std::vector<Vertex> groundVertices = {
				{{-1.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 0.5f, 1.0f}},
				{{1.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 0.5f, 1.0f}},
				{{1.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 0.5f, 1.0f}},
				{{-1.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 0.5f, 1.0f}}
			};
			std::vector<uint32_t> groundIndices = {0, 2, 1, 0, 3, 2};

			auto meshHandle = CreateComponent<MeshComponent>(groundHandle);
		}
	}

	DX::XMFLOAT3 positions[3] = {{-4.0f, 3.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {4.0f, 3.0f, 0.0f}};

	DX::XMFLOAT3 rotations[3] = {{10.0f, 15.0f, 0.0f}, {-5.0f, 120.0f, 3.0f}, {8.0f, 210.0f, -10.0f}};

	float scales[3] = {4.0f, 1.0f, 4.0f};

	for (int i = 0; i < 3; ++i)
	{
		std::string name = "Cube_" + std::to_string(i);
		auto		cubeHandle = CreateGameObject(name);
		auto*		cube = TryGetGameObject(cubeHandle);
		if (cube)
		{
			auto& tr = cube->GetTransform();
			tr.SetPosition(positions[i].x, positions[i].y, positions[i].z);
			tr.SetRotation(rotations[i].x, rotations[i].y, rotations[i].z);
			tr.SetScale(scales[i]);

			auto meshHandle = CreateComponent<MeshComponent>(cubeHandle);
		}
	}

	DEBUG_LOG_FMT("[SampleScene] Created {} GameObjects\n", 4);
}

void SampleScene::OnEndImpl() {}
