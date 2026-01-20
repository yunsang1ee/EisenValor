#include "stdafxClient.h"
#include "SampleScene.h"
#include "Component/PlayerControllerComponent.h"
#include "Component/HealthComponent.h"
#include "UI/BattleUIControllerComponent.h"
#include "Transform.h"
#include "RectTransformComponent.h"
#include "ImageUIComponent.h"
#include "ButtonUIComponent.h"
#include "UI/UITextureGlobal.h"

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
} // namespace Resources

void SampleScene::OnRegisterCustomComponents()
{
	RegisterComponents<PlayerControllerComponent, HealthComponent, BattleUIControllerComponent, RectTransformComponent, ImageUIComponent, ButtonUIComponent>();
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

	uint32_t testTextureId = UITextureGlobal::GetInstance().LoadTexture(L"\Resource\Texture\test.dds");

	ReserveGameObject(
		"Ground", std::nullopt,
		[this](GameObject* obj)
		{
			auto& tr = obj->GetTransform();
			tr.SetPosition(0.0f, -1.0f, 0.0f);
			tr.SetScale(20.0f);


			auto meshHandle = CreateComponentWithInit<MeshComponent>(
				obj->GetHandle(), [](MeshComponent* mesh)
				{ mesh->SetMesh(Ground::groundVertices, Ground::groundIndices); }
			);
		}

	);

	DX::XMFLOAT3 positions[3] = {{-4.0f, 3.0f, 0.0f}, {0.0f, 1.0f, 5.0f}, {4.0f, 3.0f, 0.0f}};

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
					obj->GetHandle(), [](MeshComponent* mesh)
					{ mesh->SetMesh(Cube::cubeVertices, Cube::cubeIndices); }
				);
			}
		);
	}

	// BattleUI와 자식 오브젝트들 생성
	ReserveGameObject(
		"BattleUI", std::nullopt,
		[this, testTextureId](GameObject* battleUIObj)
		{
			// 1. 컴포넌트 부착
			CreateComponent<BattleUIControllerComponent>(battleUIObj->GetHandle());
			auto battleUIRTrHandle = CreateComponent<RectTransformComponent>(battleUIObj->GetHandle()); // BattleUI에 RectTransformComponent 부착
			
			// BattleUI RectTransform 초기 설정
			if (RectTransformComponent* battleUIRTr = GetStorage<RectTransformComponent>()->Get(battleUIRTrHandle))
			{
				battleUIRTr->SetAnchors({0.4f, 0.4f}, {0.6f, 0.6f});
				battleUIRTr->SetOffsetMin({0.0f, 0.0f});
				battleUIRTr->SetOffsetMax({0.0f, 0.0f});
			}

			// 2. 필요한 리소스 미리 확보
			auto   parentTrHandle = battleUIObj->GetComponentHandle<Transform>();
			Scene* scene = battleUIObj->GetScene();

			if (!scene)
				return;

			// 자식 UI 레이아웃 정보
			struct UILayoutInfo { Vec2 anchorMin, anchorMax, pivot; };
			const std::unordered_map<std::string, UILayoutInfo> layoutMap = {
				{"UpUI",    {{0.5f, 0.0f}, {0.5f, 0.0f}, {0.5f, 0.0f}}},
				{"LeftUI",  {{0.0f, 0.5f}, {0.0f, 0.5f}, {0.0f, 0.5f}}},
				{"RightUI", {{1.0f, 0.5f}, {1.0f, 0.5f}, {1.0f, 0.5f}}}
			};

			for (const auto& layoutPair : layoutMap)
			{
				const std::string& name = layoutPair.first;
				const UILayoutInfo& layout = layoutPair.second;

				scene->ReserveGameObject(
					name, std::nullopt,
					[scene, parentTrHandle, testTextureId, layout](GameObject* childObj)
					{
						// RectTransform 부착
						auto childRTrHandle = scene->CreateComponent<RectTransformComponent>(childObj->GetHandle());
						
						// 자식 RectTransform 초기 설정
						if (RectTransformComponent* childRTr = scene->GetStorage<RectTransformComponent>()->Get(childRTrHandle))
						{
							childRTr->SetSizeDelta({50.0f, 50.0f}); // 고정 크기 50x50
							childRTr->SetAnchors(layout.anchorMin, layout.anchorMax);
							childRTr->SetPivot(layout.pivot);
							childRTr->SetOffsetMin({0.0f, 0.0f});
							childRTr->SetOffsetMax({0.0f, 0.0f});
						}

						// 부모 설정
						if (auto childTrHandle = childObj->GetComponentHandle<Transform>(); childTrHandle.IsValid())
						{
							if (Transform* childTr = scene->GetStorage<Transform>()->Get(childTrHandle))
							{
								childTr->SetParent(parentTrHandle);
							}
						}

						// ImageUI 컴포넌트 부착, order 및 텍스처 설정
						scene->CreateComponentWithInit<ImageUIComponent>(
							childObj->GetHandle(),
							[testTextureId](ImageUIComponent* image) {
								image->SetOrder(10);
								image->SetTexture(testTextureId);
							}
						);
						// ButtonUI 컴포넌트 부착, order 및 텍스처 설정
						scene->CreateComponentWithInit<ButtonUIComponent>(
							childObj->GetHandle(),
							[testTextureId](ButtonUIComponent* button) {
								button->SetOrder(10);
								button->SetTexture(testTextureId);
								// TODO: button->SetOnClick(...);
							}
						);
					}
				);
			}
		}
	);

	DEBUG_LOG_FMT("[SampleScene] Created {} GameObjects\n", 8);
}

void SampleScene::OnEndImpl() {}
