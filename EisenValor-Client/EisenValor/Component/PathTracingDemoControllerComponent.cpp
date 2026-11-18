#include "stdafxClient.h"
#include "PathTracingDemoControllerComponent.h"
#include "PathTracingDemoDoorComponent.h"
#include "PathTracingDemoLightComponent.h"
#include "OptionsMenuComponent.h"
#include "ButtonUIComponent.h"
#include "CameraComponent.h"
#include "DxRendererGlobal.h"
#include "DxSwapChain.h"
#include "ImageUIComponent.h"
#include "InputGlobal.h"
#include "RectTransformComponent.h"
#include "RenderPass/RaytracingPreparePass.h"
#include "ResourceGlobal.h"
#include "Scene.h"
#include "Scene/PathTracingDemoScene.h"
#include "StreamlineGlobal.h"
#include "Transform.h"
#include "Util/SceneNavigation.h"
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
#include "RenderPass/RestirDebugGlobal.h"
#endif
#include <algorithm>
#include <cmath>

namespace
{
struct DemoView
{
	const wchar_t* name;
	const wchar_t* description;
	DX::XMFLOAT3   position;
	DX::XMFLOAT3   target;
	float		   fov;
};

constexpr DemoView kViews[] = {
	{L"DRAGON ROOM",
	 L"Outside lights enter through the moving door. Space pauses; O opens/closes.",
	 {0, 2.9f, -7.1f},
	 {0, 1.8f, 1.6f},
	 58.0f},
	{L"COLOR BOUNCE",
	 L"Watch colored indirect light on the porcelain dragon and beneath the shelf.",
	 {-1.6f, 2.6f, -2.2f},
	 {-3.7f, 1.25f, 1.3f},
	 57.0f},
	{L"REFLECTIONS",
	 L"Chrome dragon and wall mirror reflect the door, colored light and bronze.",
	 {1.2f, 2.6f, -1.8f},
	 {4.0f, 1.7f, 1.3f},
	 60.0f},
	{L"ROUGHNESS",
	 L"Same metal: roughness 0.07 / 0.20 / 0.40 / 0.70 / 1.00, left to right.",
	 {0, 2.3f, -7.0f},
	 {0, 0.94f, -2.8f},
	 52.0f},
	{L"COLOR ROOM",
	 L"Three area lights overlap on white stone. L isolates RGB and two-color mixtures.",
	 {17, 3.2f, -7.1f},
	 {17, 1.4f, 0.7f},
	 62.0f},
	{L"COLORED SHADOWS",
	 L"Compare the colored penumbrae, reflected light and glowing stone joints. L changes lights.",
	 {17.6f, 4.2f, -3.7f},
	 {17, 0.6f, -0.1f},
	 62.0f},
	{L"DYNAMIC LIGHTS",
	 L"Three orbiting lights: compare moving shadows and reflections. L changes the comparison; Space pauses.",
	 {35, 3.0f, -7.1f},
	 {35, 2.0f, -0.7f},
	 62.0f},
	{L"MOVING SHADOWS",
	 L"Watch moving shadows and reflections. L cycles WHITE / FIXED RGB / RAINBOW.",
	 {39.8f, 3.2f, 1.8f},
	 {35, 1.4f, -1.25f},
	 62.0f},
};

RaytracingPreparePass* GetPreparePass()
{
	return static_cast<RaytracingPreparePass*>(GLOBAL(DxRendererGlobal).GetRenderPass("RaytracingPrepare"));
}
} // namespace

void PathTracingDemoControllerComponent::OnStart()
{
	SetView(0);
	CreateHUD();
}

void PathTracingDemoControllerComponent::SetView(uint32_t index)
{
	auto* owner = GetGameObject();
	auto* camera = owner ? owner->GetComponent<CameraComponent>() : nullptr;
	if (!camera)
	{
		return;
	}
	m_viewIndex = index % static_cast<uint32_t>(std::size(kViews));
	const auto& view = kViews[m_viewIndex];
	owner->GetTransform().SetPosition(view.position);
	camera->LookAt(view.target);
	const auto forward = owner->GetTransform().GetForward();
	m_pitch = DX::XMConvertToDegrees(-std::asin(std::clamp(forward.y, -1.0f, 1.0f)));
	m_yaw = DX::XMConvertToDegrees(std::atan2(forward.x, forward.z));
	const auto* swapChain = GLOBAL(DxRendererGlobal).GetSwapChain();
	const float aspect = swapChain && swapChain->GetHeight() > 0
							 ? static_cast<float>(swapChain->GetWidth()) / swapChain->GetHeight()
							 : 16.0f / 9.0f;
	camera->SetPerspective(DX::XMConvertToRadians(view.fov), aspect, 0.05f, 60.0f);
	GLOBAL(StreamlineGlobal).RequestHistoryReset();
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	GLOBAL(RestirDebugGlobal).RequestReferenceAccumulationReset();
	if (GLOBAL(DxRendererGlobal).IsDebugCameraFrozen())
	{
		GLOBAL(DxRendererGlobal).ToggleDebugCameraFreeze();
	}
#endif
	RefreshLabels();
}

void PathTracingDemoControllerComponent::CycleRenderer()
{
	if (OptionsMenuComponent::IsOpenInActiveScene())
	{
		return;
	}
	if (auto* prepare = GetPreparePass())
	{
		auto settings = prepare->GetControls().GetSettings();
		settings.useRestirPT = prepare->GetControls().GetPath() != RaytracingPath::RestirCandidate;
		settings.usePathTracing = true;
		prepare->GetControls().SetSettings(settings);
	}
	RefreshLabels();
}

PathTracingDemoDoorComponent* PathTracingDemoControllerComponent::GetDoor() const
{
	auto* owner = GetGameObject();
	auto* object = owner ? owner->GetScene()->TryGetGameObject(m_doorObject) : nullptr;
	return object ? object->GetComponent<PathTracingDemoDoorComponent>() : nullptr;
}

PathTracingDemoRoom PathTracingDemoControllerComponent::GetCurrentRoom() const
{
	return PathTracingDemoScene::GetRoomAtPosition(GetGameObject()->GetTransform().GetWorldPosition());
}

bool PathTracingDemoControllerComponent::CanUseRoomControls() const
{
	return GLOBAL(InputGlobal).IsWindowFocused() && !OptionsMenuComponent::IsOpenInActiveScene() &&
		   !GLOBAL(ResourceGlobal).HasPendingLoads() && !GLOBAL(StreamlineGlobal).IsFeatureWarmupActive();
}

void PathTracingDemoControllerComponent::ToggleRoomAnimation()
{
	if (!CanUseRoomControls())
		return;
	if (GetCurrentRoom() == PathTracingDemoRoom::Dragon)
	{
		if (auto* door = GetDoor())
			door->ToggleAutomatic();
	}
	else if (GetCurrentRoom() == PathTracingDemoRoom::Dynamic)
	{
		static_cast<PathTracingDemoScene*>(GetGameObject()->GetScene())->ToggleDynamicAnimation();
	}
	RefreshLabels();
}

void PathTracingDemoControllerComponent::ChangeRoomMotion()
{
	if (!CanUseRoomControls())
		return;
	if (GetCurrentRoom() == PathTracingDemoRoom::Dragon)
	{
		if (auto* door = GetDoor())
			door->ToggleOpen();
	}
	else if (GetCurrentRoom() == PathTracingDemoRoom::Dynamic)
	{
		static_cast<PathTracingDemoScene*>(GetGameObject()->GetScene())->CycleDynamicOrbitSpeed();
	}
	RefreshLabels();
}

void PathTracingDemoControllerComponent::OnUpdate(float deltaTime)
{
	m_requestedMovement = {};
	auto& input = GLOBAL(InputGlobal);
	UpdatePerformanceStats(deltaTime);
	if (auto* camera = GetGameObject()->GetComponent<CameraComponent>())
	{
		const auto* swapChain = GLOBAL(DxRendererGlobal).GetSwapChain();
		if (swapChain && swapChain->GetHeight() > 0)
		{
			const float aspect = static_cast<float>(swapChain->GetWidth()) / swapChain->GetHeight();
			if (std::abs(camera->GetAspectRatio() - aspect) > 0.0001f)
			{
				camera->SetPerspective(camera->GetFOV(), aspect, 0.05f, 60.0f);
				GLOBAL(StreamlineGlobal).RequestHistoryReset();
			}
		}
	}
	UpdateButtonAvailability();
	if (m_readyFrames < 2 && !GLOBAL(ResourceGlobal).HasPendingLoads())
	{
		if (++m_readyFrames == 2)
		{
			GLOBAL(StreamlineGlobal).SetFeatureWarmupAllowed(true);
		}
	}
	m_labelElapsed += std::max(deltaTime, 0.0f);
	if (m_labelElapsed >= 0.25f)
	{
		m_labelElapsed = 0.0f;
		RefreshLabels();
	}
	if (!input.IsWindowFocused() || OptionsMenuComponent::IsOpenInActiveScene())
	{
		input.SetMouseLocked(false);
		m_wasLooking = false;
		return;
	}
	if (input.GetUnConsumedInputDown(VK_TAB))
	{
		input.SetConsumed(VK_TAB);
		m_hudVisible = !m_hudVisible;
		if (auto* root = GetGameObject()->GetScene()->TryGetGameObject(m_hudRoot))
		{
			root->SetActive(m_hudVisible);
		}
		UpdateButtonAvailability();
	}
	for (uint32_t index = 0; index < std::size(kViews); ++index)
	{
		const auto key = static_cast<InputCode>('1' + index);
		if (input.GetUnConsumedInputDown(key))
		{
			input.SetConsumed(key);
			SetView(index);
		}
	}
	if (input.GetUnConsumedInputDown(VK_HOME))
	{
		input.SetConsumed(VK_HOME);
		SetView(0);
	}

	if (input.GetUnConsumedInputDown(VK_SPACE))
	{
		input.SetConsumed(VK_SPACE);
		ToggleRoomAnimation();
	}
	if (input.GetUnConsumedInputDown('O'))
	{
		input.SetConsumed('O');
		ChangeRoomMotion();
	}
	if (input.GetUnConsumedInputDown('L'))
	{
		input.SetConsumed('L');
		CycleRoomLighting();
	}

	const bool looking = input.GetUnConsumedInput(VK_RBUTTON);
	input.SetMouseLocked(looking);
	auto& transform = GetGameObject()->GetTransform();
	if (looking && m_wasLooking)
	{
		const auto delta = input.GetMouseDelta();
		m_yaw = std::remainder(m_yaw + delta.x * 0.12f, 360.0f);
		m_pitch = std::clamp(m_pitch + delta.y * 0.12f, -85.0f, 85.0f);
		transform.SetRotation(m_pitch, m_yaw, 0.0f);
	}
	m_wasLooking = looking;
	if (looking)
	{
		input.SetConsumed(VK_RBUTTON);
	}
	const float	 forwardInput = float(input.GetInput('W')) - float(input.GetInput('S'));
	const float	 rightInput = float(input.GetInput('D')) - float(input.GetInput('A'));
	const float	 upInput = float(input.GetInput('E')) - float(input.GetInput('Q'));
	const auto	 forward = transform.GetForward();
	const auto	 right = transform.GetRight();
	DX::XMFLOAT3 movement = {
		forward.x * forwardInput + right.x * rightInput, forward.y * forwardInput + right.y * rightInput + upInput,
		forward.z * forwardInput + right.z * rightInput
	};
	const float length = std::sqrt(movement.x * movement.x + movement.y * movement.y + movement.z * movement.z);
	if (length > 0.0001f)
	{
		const float step = (input.GetInput(VK_SHIFT) ? 6.0f : 2.0f) * std::clamp(deltaTime, 0.0f, 0.05f) / length;
		m_requestedMovement = {movement.x * step, movement.y * step, movement.z * step};
	}
}

void PathTracingDemoControllerComponent::OnLateUpdate(float deltaTime)
{
	(void)deltaTime;
	auto* owner = GetGameObject();
	auto* camera = owner ? owner->GetComponent<CameraComponent>() : nullptr;
	if (!camera)
		return;
	const float halfHeight = camera->GetNearZ() * std::tan(camera->GetFOV() * 0.5f);
	const float halfWidth = halfHeight * camera->GetAspectRatio();
	const float nearCorner =
		std::sqrt(camera->GetNearZ() * camera->GetNearZ() + halfHeight * halfHeight + halfWidth * halfWidth);
	const float radius = std::max(0.22f, nearCorner + 0.01f);
	auto&		transform = owner->GetTransform();
	const auto	position = static_cast<PathTracingDemoScene*>(owner->GetScene())
							  ->MoveCamera(transform.GetWorldPosition(), m_requestedMovement, radius);
	transform.SetWorldPosition(position);
	m_requestedMovement = {};
	if (GetCurrentRoom() != m_room)
	{
		RefreshLabels();
	}
}

void PathTracingDemoControllerComponent::CycleRoomLighting()
{
	if (!CanUseRoomControls())
		return;
	auto* scene = static_cast<PathTracingDemoScene*>(GetGameObject()->GetScene());
	if (GetCurrentRoom() == PathTracingDemoRoom::Color)
	{
		scene->CycleColorLighting();
	}
	else if (GetCurrentRoom() == PathTracingDemoRoom::Dynamic)
	{
		scene->CycleDynamicLighting();
	}
	RefreshLabels();
}

HandleOf<GameObject> PathTracingDemoControllerComponent::ReserveWidget(
	const char*				   name,
	Vec2					   anchor,
	Vec2					   minimum,
	Vec2					   maximum,
	const wchar_t*			   label,
	float					   fontSize,
	std::function<void()>	   action,
	HandleOf<TextUIComponent>* textHandle
)
{
	auto&	   scene = *GetGameObject()->GetScene();
	const auto object = scene.ReserveGameObject(
		name, std::nullopt,
		[this, &scene, anchor, minimum, maximum, label, fontSize, action, textHandle](GameObject* obj)
		{
			if (auto* root = scene.TryGetGameObject(m_hudRoot))
			{
				obj->GetTransform().SetParent(root->GetTransform().GetHandle());
			}
			scene.CreateComponentWithInit<RectTransformComponent>(
				obj->GetHandle(),
				[anchor, minimum, maximum](RectTransformComponent* rect)
				{
					rect->SetAnchors(anchor, anchor);
					rect->SetOffsetMin(minimum);
					rect->SetOffsetMax(maximum);
				}
			);
			auto image = scene.CreateComponentWithInit<ImageUIComponent>(
				obj->GetHandle(),
				[](ImageUIComponent* component)
				{
					component->SetNormalColor({0.05f, 0.05f, 0.05f, 0.94f});
					component->SetHoverColor({0.15f, 0.35f, 1.0f, 1.0f});
					component->SetPressedColor({0.15f, 0.35f, 1.0f, 1.0f});
					component->SetDisabledColor({0.05f, 0.05f, 0.05f, 0.94f});
					component->SetOrder(100);
				}
			);
			const auto text = scene.CreateComponentWithInit<TextUIComponent>(
				obj->GetHandle(),
				[label, fontSize](TextUIComponent* component)
				{
					component->SetText(label);
					component->SetFontSize(fontSize);
					component->SetHorizontalAlign(TextHorizontalAlign::Center);
					component->SetVerticalAlign(TextVerticalAlign::Center);
					component->SetColor({1, 1, 1, 1});
					component->SetOrder(101);
				}
			);
			if (textHandle)
			{
				*textHandle = text;
			}
			if (action)
			{
				scene.CreateComponentWithInit<ButtonUIComponent>(
					obj->GetHandle(),
					[this, image, action](ButtonUIComponent* button)
					{
						button->SetTargetImage(image);
						button->SetOrder(100);
						button->SetOnClick(
							[this, action]()
							{
								if (m_hudVisible && !OptionsMenuComponent::IsOpenInActiveScene())
								{
									action();
								}
							}
						);
					}
				);
			}
		}
	);
	if (action)
	{
		m_buttonObjects.push_back(object);
	}
	return object;
}

void PathTracingDemoControllerComponent::UpdateButtonAvailability()
{
	auto&	   scene = *GetGameObject()->GetScene();
	const bool enabled =
		m_hudVisible && GLOBAL(InputGlobal).IsWindowFocused() && !OptionsMenuComponent::IsOpenInActiveScene();
	const auto room = GetCurrentRoom();
	for (auto handle : m_buttonObjects)
	{
		bool available = enabled;
		if (handle == m_lightButton)
			available &=
				CanUseRoomControls() && (room == PathTracingDemoRoom::Color || room == PathTracingDemoRoom::Dynamic);
		else if (handle == m_animationButton)
			available &=
				CanUseRoomControls() && (room == PathTracingDemoRoom::Dragon || room == PathTracingDemoRoom::Dynamic);
		else if (handle == m_doorOpenButton)
			available &=
				CanUseRoomControls() && (room == PathTracingDemoRoom::Dragon || room == PathTracingDemoRoom::Dynamic);
		if (auto* object = scene.TryGetGameObject(handle))
		{
			if (auto* button = object->GetComponent<ButtonUIComponent>())
			{
				button->SetInteractable(available);
			}
			if (auto* text = object->GetComponent<TextUIComponent>())
			{
				text->SetColor(available ? DX::XMFLOAT4{1, 1, 1, 1} : DX::XMFLOAT4{0.68f, 0.68f, 0.68f, 0.3f});
			}
		}
	}
}

void PathTracingDemoControllerComponent::CreateHUD()
{
	auto& scene = *GetGameObject()->GetScene();
	m_hudRoot = scene.ReserveGameObject("DemoHUD", std::nullopt);
	ReserveWidget("DemoTitle", {0, 0}, {40, 40}, {800, 100}, L"LIGHT STUDY / DRAGON ROOM", 38, {}, &m_titleText);
	ReserveWidget("DemoDescription", {0, 0}, {40, 100}, {800, 160}, kViews[0].description, 18, {}, &m_descriptionText);
	ReserveWidget("DemoStatus", {0, 0}, {40, 160}, {800, 220}, L"Preparing lighting...", 18, {}, &m_statusText);
	ReserveWidget(
		"DemoHelp", {0.5f, 1}, {-600, -192}, {600, -144},
		L"WASD  Move    Q / E  Vertical    RMB  Look    Shift  Faster    Tab  UI    Esc  Options\n"
		L"1-4  Dragon room    5-6  Color room    7-8  Dynamic light room    L / O / Space  Current room",
		18
	);
	m_lightButton = ReserveWidget(
		"DemoRoomLights", {1, 0}, {-400, 40}, {-40, 100}, L"L / NO LIGHT CONTROL HERE", 20,
		[this]() { CycleRoomLighting(); }, &m_colorLightText
	);
	ReserveWidget(
		"DemoColorRoomJump", {1, 0}, {-400, 120}, {-40, 172}, L"COLOR ROOM  [5]", 21, [this]() { SetView(4); }
	);
	ReserveWidget(
		"DemoDynamicRoomJump", {1, 0}, {-400, 192}, {-40, 244}, L"DYNAMIC LIGHT ROOM  [7]", 21, [this]() { SetView(6); }
	);
	ReserveWidget(
		"DemoPerformance", {1, 0}, {-400, 264}, {-40, 440}, PerformanceStatsText::InitialText, 18, {},
		&m_performanceText
	);
	ReserveWidget("DemoPerformanceScope", {1, 0}, {-400, 440}, {-40, 500}, PerformanceStatsText::ScopeText, 14);
	m_animationButton = ReserveWidget(
		"DemoRoomAnimation", {0.5f, 1}, {-600, -132}, {-200, -80}, L"DOOR / AUTO", 20,
		[this]() { ToggleRoomAnimation(); }, &m_doorText
	);
	m_doorOpenButton = ReserveWidget(
		"DemoRoomMotion", {0.5f, 1}, {-180, -132}, {100, -80}, L"OPEN / CLOSE  [O]", 20,
		[this]() { ChangeRoomMotion(); }, &m_doorOpenText
	);
	ReserveWidget(
		"DemoCredit", {0.5f, 1}, {120, -132}, {600, -80}, L"Stanford Dragon / Stanford Computer Graphics Laboratory", 14
	);
	ReserveWidget(
		"DemoView", {0.5f, 1}, {-600, -64}, {-320, -12}, L"VIEW / DRAGON ROOM", 20,
		[this]() { SetView(m_viewIndex + 1); }, &m_viewText
	);
	ReserveWidget(
		"DemoRenderer", {0.5f, 1}, {-300, -64}, {40, -12}, L"RENDERER / RESTIR PT", 20, [this]() { CycleRenderer(); },
		&m_rendererText
	);
	ReserveWidget(
		"DemoOptionsButton", {0.5f, 1}, {60, -64}, {280, -12}, L"OPTIONS", 21,
		[]() { OptionsMenuComponent::OpenActive(); }
	);
	ReserveWidget(
		"DemoMenuButton", {0.5f, 1}, {300, -64}, {600, -12}, L"BACK TO MENU", 21,
		[]() { SceneNavigation::Request("StartScene"); }
	);
}

void PathTracingDemoControllerComponent::UpdatePerformanceStats(float deltaTime)
{
	if (auto value = m_performanceStats.Update(deltaTime))
	{
		if (auto* text = GetGameObject()->GetScene()->GetStorage<TextUIComponent>()->Get(m_performanceText))
		{
			text->SetText(std::move(*value));
		}
	}
}

void PathTracingDemoControllerComponent::RefreshLabels()
{
	auto* scene = GetGameObject() ? GetGameObject()->GetScene() : nullptr;
	auto* storage = scene ? scene->GetStorage<TextUIComponent>() : nullptr;
	if (!storage)
	{
		return;
	}
	auto* demo = static_cast<PathTracingDemoScene*>(scene);
	m_room = GetCurrentRoom();
	const wchar_t* roomName = L"PASSAGE";
	const wchar_t* description = L"Enter a room to use its controls. 5: color room. 7: dynamic light room.";
	const wchar_t* animationLabel = L"SPACE / NO ANIMATION HERE";
	std::wstring   lightLabel = L"L / NO LIGHT CONTROL HERE";
	switch (m_room)
	{
	case PathTracingDemoRoom::Dragon:
		roomName = L"DRAGON ROOM";
		description = kViews[0].description;
		break;
	case PathTracingDemoRoom::Color:
		roomName = L"COLOR ROOM";
		description = L"L changes the RGB mixture. The passage on the right leads to the dynamic light room.";
		lightLabel = std::format(L"COLOR / {}  [L]", demo->GetColorLightingLabel());
		break;
	case PathTracingDemoRoom::Dynamic:
		roomName = L"DYNAMIC LIGHTS";
		description = kViews[6].description;
		if (auto* light = demo->GetDynamicLight())
		{
			description = light->GetComparisonHint();
			lightLabel = std::format(L"3 LIGHTS / {}  [L]", light->GetColorLabel());
			animationLabel =
				light->IsAnimating() ? L"ORBIT + COLOR / RUNNING  [SPACE]" : L"ORBIT + COLOR / PAUSED  [SPACE]";
		}
		break;
	case PathTracingDemoRoom::Passage:
		break;
	}
	if (auto* text = storage->Get(m_titleText))
		text->SetText(std::format(L"LIGHT STUDY / {}", roomName));
	if (auto* text = storage->Get(m_viewText))
	{
		text->SetText(std::format(L"NEXT VIEW / {}  >", (m_viewIndex + 1) % std::size(kViews) + 1));
	}
	if (auto* text = storage->Get(m_descriptionText))
	{
		text->SetText(
			m_room != PathTracingDemoRoom::Dynamic &&
					GetCurrentRoom() == PathTracingDemoScene::GetRoomAtPosition(kViews[m_viewIndex].position)
				? kViews[m_viewIndex].description
				: description
		);
	}
	if (auto* text = storage->Get(m_colorLightText))
	{
		text->SetText(std::move(lightLabel));
	}
	if (auto* text = storage->Get(m_doorOpenText))
	{
		text->SetText(m_room == PathTracingDemoRoom::Dragon ? L"OPEN / CLOSE  [O]" : L"O / NO MOTION CONTROL HERE");
		if (auto* light = demo->GetDynamicLight(); light && m_room == PathTracingDemoRoom::Dynamic)
			text->SetText(std::format(L"ORBIT / {}  [O]", light->GetOrbitSpeedLabel()));
	}
	if (auto* text = storage->Get(m_doorText))
	{
		text->SetText(animationLabel);
		if (auto* door = GetDoor(); door && m_room == PathTracingDemoRoom::Dragon)
		{
			text->SetText(std::format(
				L"DOOR / {} / {:.0f}%  [SPACE]",
				door->IsAutomatic() ? L"AUTO"
				: door->IsMoving()	? L"MOVING"
									: L"PAUSED",
				door->GetOpenAmount() * 100.0f
			));
		}
	}
	if (auto* prepare = GetPreparePass())
	{
		const auto&	   controls = prepare->GetControls();
		const auto&	   settings = controls.GetSettings();
		const auto	   path = controls.GetPath();
		const wchar_t* mode = path == RaytracingPath::RestirCandidate ? L"RESTIR PT"
							  : path == RaytracingPath::Reference	  ? L"REFERENCE PT"
																	  : L"REALTIME";
		if (auto* text = storage->Get(m_rendererText))
		{
			text->SetText(std::format(L"RENDERER / {}", mode));
		}
		std::wstring status = path == RaytracingPath::Reference
								  ? L"REFERENCE PT | Pause door + orbit; keep the camera still."
								  : std::format(
										L"{} | {} SPP | {}", mode, settings.restirPrimarySpp,
										GLOBAL(StreamlineGlobal).IsRayReconstructionEnabled() ? L"RR" : L"RAW / SR"
									);
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
		const auto& debug = GLOBAL(RestirDebugGlobal);
		if (path == RaytracingPath::Reference)
		{
			status = std::format(
				L"REFERENCE PT | {} / {} frames | Pause door + orbit", debug.GetReferenceAccumulationFrames(),
				debug.GetReferenceAccumulationTarget()
			);
		}
		else if (debug.IsOverrideActive())
		{
			status =
				std::format(L"{} | {} | {} SPP", debug.GetSourceName(), debug.GetViewName(), settings.restirPrimarySpp);
		}
#endif
		if (GLOBAL(ResourceGlobal).HasPendingLoads() || GLOBAL(StreamlineGlobal).IsFeatureWarmupActive())
		{
			status = L"Preparing lighting...";
		}
		if (auto* text = storage->Get(m_statusText))
		{
			text->SetText(std::move(status));
		}
	}
	UpdateButtonAvailability();
}
