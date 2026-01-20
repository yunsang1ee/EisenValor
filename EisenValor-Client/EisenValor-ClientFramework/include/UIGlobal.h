#pragma once
#include "Singleton.h"
#include "DxCommon.h"
#include <vector>

// 전방 선언
template <typename T> 
class UIComponent;

class IUIComponent;
class Scene;

class UIGlobal : public Singleton<UIGlobal>
{
private:
	friend class Singleton<UIGlobal>;
	UIGlobal() = default;
	~UIGlobal() override = default;

public:
	void Initialize() override {}
	void Release() override {}

	// 매 프레임 UI 로직 업데이트 및 입력 처리
	// GameFramework의 Update 루프에서 호출
	void Update(float deltaTime);

private:
	// 마우스 입력 처리
	// Scene에 있는 UI 순회
	void ProcessInput(Scene* currentScene);
};
