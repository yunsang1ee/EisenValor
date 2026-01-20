#include "stdafxClientFramework.h"
#include "UIGlobal.h"
#include "SceneGlobal.h"
#include "Scene.h"
#include "InputGlobal.h"
#include "ButtonUIComponent.h"
#include "ComponentStorage.h"
#include <algorithm>

void UIGlobal::Update(float deltaTime)
{
	Scene* scene = GLOBAL(SceneGlobal).GetActiveScene();
	if (scene)
	{
		ProcessInput(scene);
	}
}

void UIGlobal::ProcessInput(Scene* currentScene)
{
	auto& input = GLOBAL(InputGlobal);

	// 현재 마우스 위치
	DirectX::XMFLOAT2 mousePos = input.GetMousePosition();

	bool isMouseDown = input.GetInput(VK_LBUTTON);
	bool isMouseUp = input.GetInputUp(VK_LBUTTON);

	// Scene의 ButtonUIComponent 목록.
	auto* bottonStorage = currentScene->GetStorage<ButtonUIComponent>();
	if (!bottonStorage)
		return;

	// 상호작용 가능한 버튼 수집
	std::vector<ButtonUIComponent*> interactables;
	for (auto& button : bottonStorage->GetList())
	{
		if (button.IsInteractable())
		{
			interactables.push_back(&button);
		}
	}

	// Z-Order
	std::sort(
		interactables.begin(), interactables.end(),
		[](ButtonUIComponent* a, ButtonUIComponent* b) { return a->GetOrder() > b->GetOrder(); }
	);

	// 입력 이벤트 전파
	for (auto* btn : interactables)
	{
		// ProcessInput true : 입력 소비
		if (btn->ProcessInput(mousePos, isMouseDown, isMouseUp))
		{
			input.SetConsumed(VK_LBUTTON);
			break;
		}
	}
}
