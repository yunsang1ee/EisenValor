#include "stdafxClient.h"
#include "PathTracingDemoControllerComponent.h"
#include "PathTracingDemoDoorComponent.h"
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

void PathTracingDemoControllerComponent::ToggleDoorAnimation()
{
	if (auto* door = GetDoor())
	{
		door->ToggleAutomatic();
	}
	RefreshLabels();
}

void PathTracingDemoControllerComponent::ToggleDoorOpen()
{
	if (auto* door = GetDoor())
	{
		door->ToggleOpen();
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
		ToggleDoorAnimation();
	}
	if (input.GetUnConsumedInputDown('O'))
	{
		input.SetConsumed('O');
		ToggleDoorOpen();
	}
	if (input.GetUnConsumedInputDown('L'))
	{
		input.SetConsumed('L');
		CycleColorLighting();
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
}

void PathTracingDemoControllerComponent::CycleColorLighting()
{
	static_cast<PathTracingDemoScene*>(GetGameObject()->GetScene())->CycleColorLighting();
	RefreshLabels();
}

void PathTracingDemoControllerComponent::ReserveWidget(
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
}

void PathTracingDemoControllerComponent::UpdateButtonAvailability()
{
	auto&	   scene = *GetGameObject()->GetScene();
	const bool enabled =
		m_hudVisible && GLOBAL(InputGlobal).IsWindowFocused() && !OptionsMenuComponent::IsOpenInActiveScene();
	for (auto handle : m_buttonObjects)
	{
		if (auto* object = scene.TryGetGameObject(handle))
		{
			if (auto* button = object->GetComponent<ButtonUIComponent>())
			{
				button->SetInteractable(enabled);
			}
		}
	}
}

void PathTracingDemoControllerComponent::CreateHUD()
{
	auto& scene = *GetGameObject()->GetScene();
	m_hudRoot = scene.ReserveGameObject("DemoHUD", std::nullopt);
	ReserveWidget("DemoTitle", {0, 0}, {40, 40}, {900, 100}, L"PATH TRACING / LIGHT STUDY", 38);
	ReserveWidget("DemoDescription", {0, 0}, {40, 100}, {900, 150}, kViews[0].description, 18, {}, &m_descriptionText);
	ReserveWidget("DemoStatus", {0, 0}, {40, 150}, {900, 200}, L"Preparing lighting...", 18, {}, &m_statusText);
	ReserveWidget(
		"DemoHelp", {0.5f, 1}, {-800, -192}, {800, -144},
		L"WASD  Move    Q / E  Vertical    RMB  Look    Shift  Faster    1-4  Dragon    5-6  Color    Tab  UI    Esc  "
		L"Options",
		18
	);
	ReserveWidget(
		"DemoColorLights", {1, 0}, {-560, 40}, {-40, 100}, L"COLOR LIGHTS / ALL RGB  [L]", 21,
		[this]() { CycleColorLighting(); }, &m_colorLightText
	);
	ReserveWidget(
		"DemoColorRoomJump", {1, 0}, {-560, 120}, {-40, 172}, L"GO TO COLOR ROOM  [5]", 21, [this]() { SetView(4); }
	);
	ReserveWidget(
		"DemoPerformance", {1, 0}, {-560, 212}, {-40, 388}, PerformanceStatsText::InitialText, 21, {},
		&m_performanceText
	);
	ReserveWidget("DemoPerformanceScope", {1, 0}, {-560, 388}, {-40, 460}, PerformanceStatsText::ScopeText, 14);
	ReserveWidget(
		"DemoDoor", {0.5f, 1}, {-800, -132}, {-300, -80}, L"DOOR / AUTO", 21, [this]() { ToggleDoorAnimation(); },
		&m_doorText
	);
	ReserveWidget(
		"DemoDoorOpen", {0.5f, 1}, {-280, -132}, {100, -80}, L"OPEN / CLOSE  [O]", 21, [this]() { ToggleDoorOpen(); }
	);
	ReserveWidget(
		"DemoCredit", {0.5f, 1}, {120, -132}, {800, -80}, L"Stanford Dragon / Stanford Computer Graphics Laboratory", 16
	);
	ReserveWidget(
		"DemoView", {0.5f, 1}, {-800, -64}, {-420, -12}, L"VIEW / DRAGON ROOM", 21,
		[this]() { SetView(m_viewIndex + 1); }, &m_viewText
	);
	ReserveWidget(
		"DemoRenderer", {0.5f, 1}, {-400, -64}, {100, -12}, L"RENDERER / RESTIR PT", 21, [this]() { CycleRenderer(); },
		&m_rendererText
	);
	ReserveWidget(
		"DemoOptionsButton", {0.5f, 1}, {120, -64}, {400, -12}, L"OPTIONS", 21,
		[]() { OptionsMenuComponent::OpenActive(); }
	);
	ReserveWidget(
		"DemoMenuButton", {0.5f, 1}, {420, -64}, {800, -12}, L"BACK TO MENU", 21,
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
	if (auto* text = storage->Get(m_viewText))
	{
		text->SetText(std::format(L"VIEW / {}", kViews[m_viewIndex].name));
	}
	if (auto* text = storage->Get(m_descriptionText))
	{
		text->SetText(kViews[m_viewIndex].description);
	}
	if (auto* text = storage->Get(m_colorLightText))
	{
		text->SetText(
			std::format(L"COLOR LIGHTS / {}  [L]", static_cast<PathTracingDemoScene*>(scene)->GetColorLightingLabel())
		);
	}
	if (auto* door = GetDoor())
	{
		if (auto* text = storage->Get(m_doorText))
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
								  ? L"REFERENCE PT | Space holds the door; keep the camera still."
								  : std::format(
										L"{} | {} SPP | {}", mode, settings.restirPrimarySpp,
										GLOBAL(StreamlineGlobal).IsRayReconstructionEnabled() ? L"RR" : L"RAW / SR"
									);
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
		const auto& debug = GLOBAL(RestirDebugGlobal);
		if (path == RaytracingPath::Reference)
		{
			status = std::format(
				L"REFERENCE PT | {} / {} frames | Space holds door", debug.GetReferenceAccumulationFrames(),
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
}
