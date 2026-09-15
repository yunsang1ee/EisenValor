#include "stdafxClient.h"
#include "WorldSceneControllerComponent.h"
#include "CameraComponent.h"
#include "DxRendererGlobal.h"
#include "DxSwapChain.h"
#include "GameObject.h"
#include "InputGlobal.h"
#include "ImageUIComponent.h"
#include "NetworkGlobal.h"
#include "RectTransformComponent.h"
#include "ResourceGlobal.h"
#include "Scene.h"
#include "SceneGlobal.h"
#include "TextureResource.h"
#include "Util/GameConstants.h"
#include "Packets/C2SPackets.h"
#include <cmath>
#include <limits>
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
#include "RenderPass/RestirDebugGlobal.h"
#endif

namespace
{
std::wstring FormatBlueScore(uint8_t score)
{
	return L"BLUE  " + std::to_wstring(score) + L" / " + std::to_wstring(MatchRule::kScoreToWin);
}

std::wstring FormatRedScore(uint8_t score)
{
	return std::to_wstring(score) + L" / " + std::to_wstring(MatchRule::kScoreToWin) + L"  RED";
}

constexpr size_t kInvalidOccupationZoneSlot = std::numeric_limits<size_t>::max();
constexpr size_t kOccupationMarkerGaugeSegmentCount = 4;
// Keep the badge one marker-height above the projected occupation-zone anchor.
constexpr float  kOccupationMarkerCenterY = -64.0f;
constexpr float  kOccupationMarkerHalfDiagonal = 34.65f;
constexpr float  kOccupationMarkerSideLength = 49.0f;
constexpr float  kOccupationMarkerGaugeThickness = 6.0f;

struct OccupationMarkerGaugeSegmentLayout
{
	Vec2  start;
	Vec2  direction;
	float rotationDegrees;
};

const std::array<OccupationMarkerGaugeSegmentLayout, kOccupationMarkerGaugeSegmentCount>
	kOccupationMarkerGaugeSegments{{
		{{0.0f, -kOccupationMarkerHalfDiagonal}, {0.70710678f, 0.70710678f}, 45.0f},
		{{kOccupationMarkerHalfDiagonal, 0.0f}, {-0.70710678f, 0.70710678f}, -45.0f},
		{{0.0f, kOccupationMarkerHalfDiagonal}, {-0.70710678f, -0.70710678f}, 45.0f},
		{{-kOccupationMarkerHalfDiagonal, 0.0f}, {0.70710678f, -0.70710678f}, -45.0f},
	}};

DirectX::XMFLOAT4 GetOccupationTeamColor(FB_ENUMS::TEAM_TYPE team, float alpha = 1.0f)
{
	switch (team)
	{
	case FB_ENUMS::TEAM_TYPE_BLUE:
		return {0.15f, 0.35f, 1.0f, alpha};
	case FB_ENUMS::TEAM_TYPE_RED:
		return {1.0f, 0.2f, 0.2f, alpha};
	default:
		return {0.85f, 0.72f, 0.32f, alpha};
	}
}

} // namespace

void OccupationZoneRegistrationComponent::OnStart()
{
	auto* scene = GetGameObject()->GetScene();
	if (!scene)
	{
		return;
	}

	auto* controllerStorage = scene->GetStorage<WorldSceneControllerComponent>();
	if (!controllerStorage || controllerStorage->GetList().empty())
	{
		return;
	}

	auto&		 controller = *controllerStorage->GetList().begin();
	const uint64 zoneID = GetGameObject()->GetServerID();
	const Vec3	 position = GetGameObject()->GetTransform().GetWorldPosition();
	controller.RegisterOccupationZone(zoneID, position);
	DEBUG_LOG_FMT(
		"[OccupationZoneUI] Registered zone ID:{} at ({:.1f}, {:.1f}, {:.1f})\n", zoneID, position.x, position.y,
		position.z
	);
}

void WorldSceneControllerComponent::OnStart()
{
	CreateTeamScoreUI();
	CreateOccupationZoneUI();
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	CreateRestirDebugOverlayUI();
#endif
}

void WorldSceneControllerComponent::OnUpdate(float deltaTime)
{
	if (m_scoreTextDirty)
	{
		m_scoreTextDirty = !RefreshTeamScoreText();
	}
	UpdateOccupationZoneUI(deltaTime);
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	RefreshRestirDebugOverlay();
#endif

#ifdef APPLY_LOBBY_SERVER
	if (GLOBAL(InputGlobal).GetInputDown('L'))
	{
		GLOBAL(NetBridge::NetworkGlobal).DisconnectGameServer();
		GLOBAL(NetBridge::NetworkGlobal).ReconnectLobbyServer();
		auto pb{NetBridge::C2S::Make_CL_RETURN_TO_GAME_ROOM_PACKET(GLOBAL(SceneGlobal).GetSessionID())};
		GLOBAL(NetBridge::NetworkGlobal).SendLobby(std::move(pb));
	}
#endif
}

#if defined(ENABLE_RENDER_DEBUG_VIEWS)
void WorldSceneControllerComponent::CreateRestirDebugOverlayUI()
{
	auto* scene = GetGameObject()->GetScene();
	if (!scene)
	{
		return;
	}

	scene->ReserveGameObject(
		"RestirDebugOverlay", std::nullopt,
		[this, scene](GameObject* obj)
		{
			scene->CreateComponentWithInit<RectTransformComponent>(
				obj->GetHandle(),
				[](RectTransformComponent* rect)
				{
					rect->SetAnchors({1.0f, 0.0f}, {1.0f, 0.0f});
					rect->SetPivot({1.0f, 0.0f});
					rect->SetOffsetMin({-470.0f, 18.0f});
					rect->SetOffsetMax({-18.0f, 132.0f});
				}
			);

			scene->CreateComponentWithInit<ImageUIComponent>(
				obj->GetHandle(),
				[](ImageUIComponent* image)
				{
					auto texture = GLOBAL(ResourceGlobal).Load<TextureResource>(L"Resource\\Texture\\UIback.evtex");
					image->SetNormalTextureResource(texture);
					image->SetNormalColor({0.18f, 0.16f, 0.12f, 0.92f});
					image->SetOrder(100010);
				}
			);

			m_restirDebugTextHandle = scene->CreateComponentWithInit<TextUIComponent>(
				obj->GetHandle(),
				[](TextUIComponent* text)
				{
					text->SetText(L"");
					text->SetFontSize(16.0f);
					text->SetHorizontalAlign(TextHorizontalAlign::Left);
					text->SetVerticalAlign(TextVerticalAlign::Center);
					text->SetColor({0.93f, 0.86f, 0.65f, 1.0f});
					text->SetOrder(100011);
				}
			);
			obj->SetActive(false);
		}
	);
}

bool WorldSceneControllerComponent::RefreshRestirDebugOverlay()
{
	auto* scene = GetGameObject()->GetScene();
	if (!scene)
	{
		return false;
	}

	auto* textStorage = scene->GetStorage<TextUIComponent>();
	if (!textStorage)
	{
		return false;
	}

	auto* text = textStorage->Get(m_restirDebugTextHandle);
	if (!text)
	{
		return false;
	}

	auto& debug = GLOBAL(RestirDebugGlobal);
	if (m_lastRestirDebugRevision == debug.GetRevision())
	{
		return true;
	}

	auto* owner = text->GetGameObject();
	if (!owner)
	{
		return false;
	}

	const bool visible = debug.IsOverrideActive();
	owner->SetActive(visible);
	if (visible)
	{
		std::wstring status = L"  SOURCE : ";
		status += debug.GetSourceName();
		status += L"\n  VIEW   : ";
		status += debug.GetViewName();
		status += L"\n  DLSS   : ";
		status += debug.BypassDlss() ? L"BYPASSED" : L"REQUESTED";
		status += L"\n  F12 SOURCE  |  F10 VIEW  |  SHIFT REVERSE";
		text->SetText(std::move(status));
	}
	m_lastRestirDebugRevision = debug.GetRevision();
	return true;
}
#endif

void WorldSceneControllerComponent::SetTeamScores(uint8_t blueScore, uint8_t redScore)
{
	m_blueScore = blueScore;
	m_redScore = redScore;
	m_scoreTextDirty = true;
}

void WorldSceneControllerComponent::RegisterOccupationZone(uint64 zoneID, const Vec3& position)
{
	auto& state = m_occupationZones[zoneID];
	state.position = position;
	state.registered = true;
	ResolveOccupationZoneOrder();
}

void WorldSceneControllerComponent::SetOccupationZoneOwner(uint64 zoneID, FB_ENUMS::TEAM_TYPE ownerTeam)
{
	auto& state = m_occupationZones[zoneID];
	state.ownerTeam = ownerTeam;

	if (ownerTeam == FB_ENUMS::TEAM_TYPE_NONE || state.dominantTeam == ownerTeam)
	{
		state.graceActive = false;
		state.graceRemaining = 0.0f;
		state.displayedGraceTenths = -1;
	}

	const size_t slot = FindOccupationZoneSlot(zoneID);
	if (slot != kInvalidOccupationZoneSlot)
	{
		UpdateOccupationOwnerStyle(slot);
	}
}

void WorldSceneControllerComponent::SetOccupationZoneGauge(uint64 zoneID, float gauge, FB_ENUMS::TEAM_TYPE dominantTeam)
{
	auto&		state = m_occupationZones[zoneID];
	const float clampedGauge = std::clamp(gauge, -100.0f, 100.0f);
	const bool	gaugeChanged = state.hasGauge && std::abs(clampedGauge - state.gauge) > 0.05f;
	const auto	previousDominantTeam = state.dominantTeam;
	const bool	enemyDominant = state.ownerTeam != FB_ENUMS::TEAM_TYPE_NONE &&
							   dominantTeam != FB_ENUMS::TEAM_TYPE_NONE && dominantTeam != state.ownerTeam;

	if (!enemyDominant)
	{
		state.graceActive = false;
		state.graceRemaining = 0.0f;
		state.displayedGraceTenths = -1;
	}
	else if (!state.hasGauge || previousDominantTeam != dominantTeam)
	{
		state.graceActive = true;
		state.graceRemaining = kRecaptureGraceSeconds;
		state.displayedGraceTenths = -1;
	}
	else if (state.graceActive && gaugeChanged)
	{
		// The server only changes the gauge after its grace period has completed.
		state.graceActive = false;
		state.graceRemaining = 0.0f;
		state.displayedGraceTenths = -1;
	}

	state.gauge = clampedGauge;
	state.dominantTeam = dominantTeam;
	state.hasGauge = true;
	state.markerGaugeDirty = true;

	const size_t slot = FindOccupationZoneSlot(zoneID);
	if (slot != kInvalidOccupationZoneSlot)
	{
		UpdateOccupationGauge(slot);
	}
}

void WorldSceneControllerComponent::CreateOccupationZoneUI()
{
	for (size_t slot = 0; slot < kOccupationZoneCount; ++slot)
	{
		CreateOccupationZoneMarker(slot);
		CreateOccupationZoneGraceText(slot);
	}
	BindOccupationGaugeUI();
}

void WorldSceneControllerComponent::CreateOccupationZoneMarker(size_t slot)
{
	auto* scene = GetGameObject()->GetScene();
	if (!scene)
	{
		return;
	}

	const std::string rootName = "OccupationZoneMarker" + std::to_string(slot);
	scene->ReserveGameObject(
		rootName, std::nullopt,
		[this, scene, slot, rootName](GameObject* root)
		{
			scene->CreateComponentWithInit<RectTransformComponent>(
				root->GetHandle(),
				[](RectTransformComponent* rect)
				{
					rect->SetAnchors({0.5f, 0.5f}, {0.5f, 0.5f});
					rect->SetPivot({0.5f, 0.5f});
					rect->SetOffsetMin({0.0f, 0.0f});
					rect->SetOffsetMax({0.0f, 0.0f});
				}
			);

			const auto rootHandle = root->GetHandle();
			m_occupationZoneUI[slot].markerRootHandle = rootHandle;

			scene->ReserveGameObject(
				rootName + "Badge", std::nullopt,
				[this, scene, slot, rootHandle](GameObject* child)
				{
					child->GetTransform().SetParent(scene->TryGetGameObject(rootHandle)->GetComponentHandle<Transform>()
					);
					scene->CreateComponentWithInit<RectTransformComponent>(
						child->GetHandle(),
						[](RectTransformComponent* rect)
						{
							rect->SetAnchors({0.5f, 0.5f}, {0.5f, 0.5f});
							rect->SetPivot({0.5f, 0.5f});
							rect->SetOffsetMin({-28.0f, kOccupationMarkerCenterY - 28.0f});
							rect->SetOffsetMax({28.0f, kOccupationMarkerCenterY + 28.0f});
							rect->SetRotationDegrees(45.0f);
						}
					);
					m_occupationZoneUI[slot].markerBadgeHandle = scene->CreateComponentWithInit<ImageUIComponent>(
						child->GetHandle(),
						[](ImageUIComponent* image)
						{
							image->SetNormalColor(GetOccupationTeamColor(FB_ENUMS::TEAM_TYPE_NONE, 0.42f));
							image->SetOrder(99982);
						}
					);
				}
			);

			scene->ReserveGameObject(
				rootName + "BadgeInner", std::nullopt,
				[this, scene, slot, rootHandle](GameObject* child)
				{
					child->GetTransform().SetParent(scene->TryGetGameObject(rootHandle)->GetComponentHandle<Transform>()
					);
					scene->CreateComponentWithInit<RectTransformComponent>(
						child->GetHandle(),
						[](RectTransformComponent* rect)
						{
							rect->SetAnchors({0.5f, 0.5f}, {0.5f, 0.5f});
							rect->SetPivot({0.5f, 0.5f});
							rect->SetOffsetMin({-21.0f, kOccupationMarkerCenterY - 21.0f});
							rect->SetOffsetMax({21.0f, kOccupationMarkerCenterY + 21.0f});
							rect->SetRotationDegrees(45.0f);
						}
					);
					m_occupationZoneUI[slot].markerBadgeInnerHandle = scene->CreateComponentWithInit<ImageUIComponent>(
						child->GetHandle(),
						[](ImageUIComponent* image)
						{
							image->SetNormalColor({0.03f, 0.03f, 0.03f, 0.68f});
							image->SetOrder(99983);
						}
					);
				}
			);

			for (size_t segment = 0; segment < kOccupationMarkerGaugeSegmentCount; ++segment)
			{
				scene->ReserveGameObject(
					rootName + "GaugeSegment" + std::to_string(segment), std::nullopt,
					[this, scene, slot, segment, rootHandle](GameObject* child)
					{
						child->GetTransform().SetParent(
							scene->TryGetGameObject(rootHandle)->GetComponentHandle<Transform>()
						);
						scene->CreateComponentWithInit<RectTransformComponent>(
							child->GetHandle(),
							[segment](RectTransformComponent* rect)
							{
								rect->SetAnchors({0.5f, 0.5f}, {0.5f, 0.5f});
								rect->SetPivot({0.5f, 0.5f});
								rect->SetOffsetMin({0.0f, 0.0f});
								rect->SetOffsetMax({0.0f, 0.0f});
								rect->SetRotationDegrees(kOccupationMarkerGaugeSegments[segment].rotationDegrees);
							}
						);
						m_occupationZoneUI[slot].markerGaugeSegmentHandles[segment] =
							scene->CreateComponentWithInit<ImageUIComponent>(
								child->GetHandle(),
								[](ImageUIComponent* image)
								{
									image->SetNormalColor(GetOccupationTeamColor(FB_ENUMS::TEAM_TYPE_NONE, 0.78f));
									image->SetOrder(99984);
								}
							);
						child->SetActive(false);
					}
				);
			}

			scene->ReserveGameObject(
				rootName + "Label", std::nullopt,
				[this, scene, slot, rootHandle](GameObject* child)
				{
					child->GetTransform().SetParent(scene->TryGetGameObject(rootHandle)->GetComponentHandle<Transform>()
					);
					scene->CreateComponentWithInit<RectTransformComponent>(
						child->GetHandle(),
						[](RectTransformComponent* rect)
						{
							rect->SetAnchors({0.5f, 0.5f}, {0.5f, 0.5f});
							rect->SetPivot({0.5f, 0.5f});
							rect->SetOffsetMin({-38.0f, kOccupationMarkerCenterY - 38.0f});
							rect->SetOffsetMax({38.0f, kOccupationMarkerCenterY + 38.0f});
						}
					);
					m_occupationZoneUI[slot].markerLabelHandle = scene->CreateComponentWithInit<TextUIComponent>(
						child->GetHandle(),
						[slot](TextUIComponent* text)
						{
							text->SetText(slot == 0 ? L"A" : L"B");
							text->SetFontSize(32.0f);
							text->SetHorizontalAlign(TextHorizontalAlign::Center);
							text->SetVerticalAlign(TextVerticalAlign::Center);
							text->SetColor({1.0f, 1.0f, 1.0f, 0.9f});
							text->SetOrder(99985);
						}
					);
					UpdateOccupationOwnerStyle(slot);
					UpdateOccupationMarkerGauge(slot);
				}
			);

			root->SetActive(false);
		}
	);
}

void WorldSceneControllerComponent::CreateOccupationZoneGraceText(size_t slot)
{
	auto* scene = GetGameObject()->GetScene();
	if (!scene)
	{
		return;
	}

	scene->ReserveGameObject(
		"OccupationZoneGrace" + std::to_string(slot), std::nullopt,
		[this, scene, slot](GameObject* obj)
		{
			const float y = 22.0f + static_cast<float>(slot) * 36.0f;
			scene->CreateComponentWithInit<RectTransformComponent>(
				obj->GetHandle(),
				[y](RectTransformComponent* rect)
				{
					rect->SetAnchors({0.0f, 0.0f}, {0.0f, 0.0f});
					rect->SetPivot({0.0f, 0.0f});
					rect->SetOffsetMin({116.0f, y});
					rect->SetOffsetMax({292.0f, y + 28.0f});
				}
			);
			scene->CreateComponentWithInit<ImageUIComponent>(
				obj->GetHandle(),
				[](ImageUIComponent* image)
				{
					image->SetNormalColor({0.03f, 0.03f, 0.03f, 0.9f});
					image->SetOrder(99993);
				}
			);
			m_occupationZoneUI[slot].graceTextHandle = scene->CreateComponentWithInit<TextUIComponent>(
				obj->GetHandle(),
				[](TextUIComponent* text)
				{
					text->SetText(L"");
					text->SetFontSize(14.0f);
					text->SetHorizontalAlign(TextHorizontalAlign::Center);
					text->SetVerticalAlign(TextVerticalAlign::Center);
					text->SetColor(GetOccupationTeamColor(FB_ENUMS::TEAM_TYPE_NONE));
					text->SetOrder(99994);
				}
			);
			m_occupationZoneUI[slot].graceRootHandle = obj->GetHandle();
			obj->SetActive(false);
		}
	);
}

void WorldSceneControllerComponent::ResolveOccupationZoneOrder()
{
	if (m_occupationZoneOrderReady)
	{
		return;
	}

	uint64 westZoneID = 0;
	uint64 eastZoneID = 0;
	float  westX = std::numeric_limits<float>::max();
	float  eastX = std::numeric_limits<float>::lowest();
	size_t registeredCount = 0;
	for (const auto& [zoneID, state] : m_occupationZones)
	{
		if (!state.registered)
		{
			continue;
		}

		++registeredCount;
		if (state.position.x < westX)
		{
			westX = state.position.x;
			westZoneID = zoneID;
		}
		if (state.position.x > eastX)
		{
			eastX = state.position.x;
			eastZoneID = zoneID;
		}
	}

	if (registeredCount < kOccupationZoneCount || westZoneID == eastZoneID)
	{
		return;
	}

	// The current map contract defines WEST as A and EAST as B.
	m_occupationZoneIDs[0] = westZoneID;
	m_occupationZoneIDs[1] = eastZoneID;
	m_occupationZoneOrderReady = true;
	DEBUG_LOG_FMT("[OccupationZoneUI] A(WEST) ID:{}, B(EAST) ID:{}\n", westZoneID, eastZoneID);

	for (size_t slot = 0; slot < kOccupationZoneCount; ++slot)
	{
		UpdateOccupationGauge(slot);
		UpdateOccupationOwnerStyle(slot);
	}
}

void WorldSceneControllerComponent::BindOccupationGaugeUI()
{
	if (m_occupationGaugeUIBound)
	{
		return;
	}

	auto* scene = GetGameObject()->GetScene();
	if (!scene)
	{
		return;
	}

	auto* imageStorage = scene->GetStorage<ImageUIComponent>();
	auto* textStorage = scene->GetStorage<TextUIComponent>();
	if (!imageStorage || !textStorage)
	{
		return;
	}

	for (auto& image : imageStorage->GetList())
	{
		auto* owner = image.GetGameObject();
		if (!owner)
		{
			continue;
		}

		const auto& name = owner->GetName();
		if (name == "OccupationGaugeBlue")
		{
			m_occupationZoneUI[0].gaugeBlueFillHandle = image.GetHandle();
		}
		else if (name == "OccupationGaugeRed")
		{
			m_occupationZoneUI[0].gaugeRedFillHandle = image.GetHandle();
		}
		else if (name == "OccupationGaugeBlueB")
		{
			m_occupationZoneUI[1].gaugeBlueFillHandle = image.GetHandle();
		}
		else if (name == "OccupationGaugeRedB")
		{
			m_occupationZoneUI[1].gaugeRedFillHandle = image.GetHandle();
		}
		else if (name == "OccupationGaugeLabel0")
		{
			m_occupationZoneUI[0].gaugeLabelBackgroundHandle = image.GetHandle();
		}
		else if (name == "OccupationGaugeLabel1")
		{
			m_occupationZoneUI[1].gaugeLabelBackgroundHandle = image.GetHandle();
		}
	}

	for (auto& text : textStorage->GetList())
	{
		auto* owner = text.GetGameObject();
		if (!owner)
		{
			continue;
		}

		if (owner->GetName() == "OccupationGaugeLabel0")
		{
			m_occupationZoneUI[0].gaugeLabelHandle = text.GetHandle();
		}
		else if (owner->GetName() == "OccupationGaugeLabel1")
		{
			m_occupationZoneUI[1].gaugeLabelHandle = text.GetHandle();
		}
	}

	m_occupationGaugeUIBound =
		m_occupationZoneUI[0].gaugeBlueFillHandle.IsValid() && m_occupationZoneUI[0].gaugeRedFillHandle.IsValid() &&
		m_occupationZoneUI[1].gaugeBlueFillHandle.IsValid() && m_occupationZoneUI[1].gaugeRedFillHandle.IsValid() &&
		m_occupationZoneUI[0].gaugeLabelHandle.IsValid() && m_occupationZoneUI[1].gaugeLabelHandle.IsValid();

	if (m_occupationGaugeUIBound)
	{
		for (size_t slot = 0; slot < kOccupationZoneCount; ++slot)
		{
			UpdateOccupationGauge(slot);
			UpdateOccupationOwnerStyle(slot);
		}
	}
}

void WorldSceneControllerComponent::UpdateOccupationZoneUI(float deltaTime)
{
	BindOccupationGaugeUI();
	if (!m_occupationZoneOrderReady)
	{
		ResolveOccupationZoneOrder();
	}

	auto* scene = GetGameObject()->GetScene();
	if (!scene)
	{
		return;
	}

	for (size_t slot = 0; slot < kOccupationZoneCount; ++slot)
	{
		UpdateOccupationMarker(slot);

		const uint64 zoneID = m_occupationZoneIDs[slot];
		auto		 stateIter = m_occupationZones.find(zoneID);
		if (!m_occupationZoneOrderReady || stateIter == m_occupationZones.end())
		{
			continue;
		}

		auto& state = stateIter->second;
		if (state.markerGaugeDirty)
		{
			UpdateOccupationMarkerGauge(slot);
		}
		if (state.graceActive)
		{
			state.graceRemaining = std::max(0.0f, state.graceRemaining - deltaTime);
			state.graceActive = state.graceRemaining > 0.0f;
		}

		auto* graceRoot = scene->TryGetGameObject(m_occupationZoneUI[slot].graceRootHandle);
		if (graceRoot)
		{
			graceRoot->SetActive(state.graceActive);
		}

		if (!state.graceActive)
		{
			continue;
		}

		const int tenths = static_cast<int>(std::ceil(state.graceRemaining * 10.0f));
		if (tenths != state.displayedGraceTenths)
		{
			if (auto* text = scene->GetStorage<TextUIComponent>()->Get(m_occupationZoneUI[slot].graceTextHandle))
			{
				text->SetText(
					L"\uC7AC\uC810\uB839 " + std::to_wstring(tenths / 10) + L"." + std::to_wstring(tenths % 10) +
					L"\uCD08"
				);
				text->SetColor(GetOccupationTeamColor(state.dominantTeam));
				state.displayedGraceTenths = tenths;
			}
		}
	}
}

void WorldSceneControllerComponent::UpdateOccupationGauge(size_t slot)
{
	if (!m_occupationZoneOrderReady || slot >= kOccupationZoneCount)
	{
		return;
	}

	auto* scene = GetGameObject()->GetScene();
	if (!scene)
	{
		return;
	}

	auto stateIter = m_occupationZones.find(m_occupationZoneIDs[slot]);
	if (stateIter == m_occupationZones.end())
	{
		return;
	}

	const float blueAmount = std::max(-stateIter->second.gauge, 0.0f) / 100.0f;
	const float redAmount = std::max(stateIter->second.gauge, 0.0f) / 100.0f;
	auto*		imageStorage = scene->GetStorage<ImageUIComponent>();
	if (!imageStorage)
	{
		return;
	}

	if (auto* image = imageStorage->Get(m_occupationZoneUI[slot].gaugeBlueFillHandle))
	{
		if (auto* rect = image->GetGameObject()->GetComponent<RectTransformComponent>())
		{
			rect->SetAnchors({0.5f - 0.5f * blueAmount, 0.0f}, {0.5f, 1.0f});
			rect->SetOffsetMin({0.0f, 2.0f});
			rect->SetOffsetMax({0.0f, -2.0f});
		}
	}

	if (auto* image = imageStorage->Get(m_occupationZoneUI[slot].gaugeRedFillHandle))
	{
		if (auto* rect = image->GetGameObject()->GetComponent<RectTransformComponent>())
		{
			rect->SetAnchors({0.5f, 0.0f}, {0.5f + 0.5f * redAmount, 1.0f});
			rect->SetOffsetMin({0.0f, 2.0f});
			rect->SetOffsetMax({0.0f, -2.0f});
		}
	}
}

void WorldSceneControllerComponent::UpdateOccupationMarker(size_t slot)
{
	auto* scene = GetGameObject()->GetScene();
	if (!scene || slot >= kOccupationZoneCount)
	{
		return;
	}

	auto* markerRoot = scene->TryGetGameObject(m_occupationZoneUI[slot].markerRootHandle);
	if (!markerRoot)
	{
		return;
	}

	if (!m_occupationZoneOrderReady)
	{
		markerRoot->SetActive(false);
		return;
	}

	auto  stateIter = m_occupationZones.find(m_occupationZoneIDs[slot]);
	auto* swapChain = GLOBAL(DxRendererGlobal).GetSwapChain();
	if (stateIter == m_occupationZones.end() || !stateIter->second.registered || !swapChain ||
		!CameraComponent::GetMainCamera())
	{
		markerRoot->SetActive(false);
		return;
	}

	DirectX::XMFLOAT3 worldPosition = stateIter->second.position;
	worldPosition.y += 1.0f;
	const DirectX::XMMATRIX view = CameraComponent::GetMainViewMatrix();
	const DirectX::XMMATRIX projection = CameraComponent::GetMainProjectionMatrix();
	const DirectX::XMVECTOR worldPositionVector = DirectX::XMLoadFloat3(&worldPosition);
	const DirectX::XMVECTOR viewPosition = DirectX::XMVector3TransformCoord(worldPositionVector, view);
	const float				viewDepth = DirectX::XMVectorGetZ(viewPosition);
	const DirectX::XMVECTOR projectedPosition = DirectX::XMVector3Project(
		worldPositionVector, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, projection, view, DirectX::XMMatrixIdentity()
	);

	DirectX::XMFLOAT3 screenPosition;
	DirectX::XMStoreFloat3(&screenPosition, projectedPosition);
	const bool visible = viewDepth > 0.0f && screenPosition.x >= 0.0f && screenPosition.x <= 1.0f &&
						 screenPosition.y >= 0.0f && screenPosition.y <= 1.0f && screenPosition.z >= 0.0f &&
						 screenPosition.z <= 1.0f;
	markerRoot->SetActive(visible);
	if (!visible)
	{
		return;
	}

	if (auto* rect = markerRoot->GetComponent<RectTransformComponent>())
	{
		const float x = (screenPosition.x - 0.5f) * static_cast<float>(swapChain->GetWidth());
		const float y = (screenPosition.y - 0.5f) * static_cast<float>(swapChain->GetHeight());
		rect->SetOffsetMin({x, y});
		rect->SetOffsetMax({x, y});
	}
}

void WorldSceneControllerComponent::UpdateOccupationMarkerGauge(size_t slot)
{
	if (!m_occupationZoneOrderReady || slot >= kOccupationZoneCount)
	{
		return;
	}

	auto* scene = GetGameObject()->GetScene();
	if (!scene)
	{
		return;
	}

	auto stateIter = m_occupationZones.find(m_occupationZoneIDs[slot]);
	if (stateIter == m_occupationZones.end())
	{
		return;
	}

	auto* imageStorage = scene->GetStorage<ImageUIComponent>();
	if (!imageStorage)
	{
		return;
	}

	auto& state = stateIter->second;
	const float gaugeAmount = state.hasGauge ? std::clamp(std::abs(state.gauge) / 100.0f, 0.0f, 1.0f) : 0.0f;
	const auto  gaugeTeam = state.gauge < 0.0f ? FB_ENUMS::TEAM_TYPE_BLUE
											  : state.gauge > 0.0f ? FB_ENUMS::TEAM_TYPE_RED
															 : FB_ENUMS::TEAM_TYPE_NONE;
	bool updatedAllSegments = true;

	for (size_t segment = 0; segment < kOccupationMarkerGaugeSegmentCount; ++segment)
	{
		auto* image = imageStorage->Get(m_occupationZoneUI[slot].markerGaugeSegmentHandles[segment]);
		if (!image || !image->GetGameObject())
		{
			updatedAllSegments = false;
			continue;
		}

		auto* segmentObject = image->GetGameObject();
		const float segmentAmount = std::clamp(
			gaugeAmount * static_cast<float>(kOccupationMarkerGaugeSegmentCount) - static_cast<float>(segment),
			0.0f, 1.0f
		);
		if (segmentAmount <= 0.0f)
		{
			segmentObject->SetActive(false);
			continue;
		}

		auto* rect = segmentObject->GetComponent<RectTransformComponent>();
		if (!rect)
		{
			updatedAllSegments = false;
			continue;
		}

		const auto& layout = kOccupationMarkerGaugeSegments[segment];
		const float length = kOccupationMarkerSideLength * segmentAmount;
		const Vec2 center = {
			layout.start.x + layout.direction.x * length * 0.5f,
			kOccupationMarkerCenterY + layout.start.y + layout.direction.y * length * 0.5f,
		};
		rect->SetOffsetMin({center.x - length * 0.5f, center.y - kOccupationMarkerGaugeThickness * 0.5f});
		rect->SetOffsetMax({center.x + length * 0.5f, center.y + kOccupationMarkerGaugeThickness * 0.5f});
		image->SetNormalColor(GetOccupationTeamColor(gaugeTeam, 0.78f));
		segmentObject->SetActive(true);
	}

	state.markerGaugeDirty = !updatedAllSegments;
}

void WorldSceneControllerComponent::UpdateOccupationOwnerStyle(size_t slot)
{
	if (!m_occupationZoneOrderReady || slot >= kOccupationZoneCount)
	{
		return;
	}

	auto* scene = GetGameObject()->GetScene();
	if (!scene)
	{
		return;
	}

	auto stateIter = m_occupationZones.find(m_occupationZoneIDs[slot]);
	if (stateIter == m_occupationZones.end())
	{
		return;
	}

	const auto ownerTeam = stateIter->second.ownerTeam;
	auto*	   imageStorage = scene->GetStorage<ImageUIComponent>();
	auto*	   textStorage = scene->GetStorage<TextUIComponent>();
	if (imageStorage)
	{
		if (auto* image = imageStorage->Get(m_occupationZoneUI[slot].markerBadgeHandle))
		{
			image->SetNormalColor(GetOccupationTeamColor(ownerTeam, 0.42f));
		}
		if (auto* image = imageStorage->Get(m_occupationZoneUI[slot].gaugeLabelBackgroundHandle))
		{
			const DirectX::XMFLOAT4 color = ownerTeam == FB_ENUMS::TEAM_TYPE_NONE
												? DirectX::XMFLOAT4{0.05f, 0.05f, 0.05f, 0.95f}
												: GetOccupationTeamColor(ownerTeam, 0.9f);
			image->SetNormalColor(color);
		}
	}

	if (textStorage)
	{
		if (auto* text = textStorage->Get(m_occupationZoneUI[slot].markerLabelHandle))
		{
			text->SetColor({1.0f, 1.0f, 1.0f, 0.9f});
		}
		if (auto* text = textStorage->Get(m_occupationZoneUI[slot].gaugeLabelHandle))
		{
			text->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
		}
	}
}

size_t WorldSceneControllerComponent::FindOccupationZoneSlot(uint64 zoneID) const
{
	if (!m_occupationZoneOrderReady)
	{
		return kInvalidOccupationZoneSlot;
	}

	if (m_occupationZoneIDs[0] == zoneID)
	{
		return 0;
	}
	if (m_occupationZoneIDs[1] == zoneID)
	{
		return 1;
	}
	return kInvalidOccupationZoneSlot;
}

void WorldSceneControllerComponent::CreateTeamScoreUI()
{
	auto* scene = GetGameObject()->GetScene();
	if (!scene)
	{
		return;
	}

	scene->ReserveGameObject(
		"TeamScorePanel", std::nullopt,
		[scene](GameObject* obj)
		{
			scene->CreateComponentWithInit<RectTransformComponent>(
				obj->GetHandle(),
				[](RectTransformComponent* rect)
				{
					rect->SetAnchors({0.5f, 0.0f}, {0.5f, 0.0f});
					rect->SetPivot({0.5f, 0.0f});
					rect->SetOffsetMin({-360.0f, 16.0f});
					rect->SetOffsetMax({360.0f, 60.0f});
				}
			);

			scene->CreateComponentWithInit<ImageUIComponent>(
				obj->GetHandle(),
				[](ImageUIComponent* image)
				{
					auto texture = GLOBAL(ResourceGlobal).Load<TextureResource>(L"Resource\\Texture\\UIback.evtex");
					image->SetNormalTextureResource(texture);
					image->SetNormalColor({1.0f, 1.0f, 1.0f, 0.95f});
					image->SetOrder(99994);
				}
			);
		}
	);

	scene->ReserveGameObject(
		"BlueTeamScoreText", std::nullopt,
		[this, scene](GameObject* obj)
		{
			scene->CreateComponentWithInit<RectTransformComponent>(
				obj->GetHandle(),
				[](RectTransformComponent* rect)
				{
					rect->SetAnchors({0.5f, 0.0f}, {0.5f, 0.0f});
					rect->SetPivot({0.5f, 0.0f});
					rect->SetOffsetMin({-344.0f, 16.0f});
					rect->SetOffsetMax({-72.0f, 60.0f});
				}
			);

			m_blueTeamScoreTextHandle = scene->CreateComponentWithInit<TextUIComponent>(
				obj->GetHandle(),
				[this](TextUIComponent* text)
				{
					text->SetText(FormatBlueScore(m_blueScore));
					text->SetFontSize(23.0f);
					text->SetHorizontalAlign(TextHorizontalAlign::Center);
					text->SetVerticalAlign(TextVerticalAlign::Center);
					text->SetColor({0.35f, 0.55f, 1.0f, 1.0f});
					text->SetOrder(99995);
				}
			);
		}
	);

	scene->ReserveGameObject(
		"TeamScoreDivider", std::nullopt,
		[scene](GameObject* obj)
		{
			scene->CreateComponentWithInit<RectTransformComponent>(
				obj->GetHandle(),
				[](RectTransformComponent* rect)
				{
					rect->SetAnchors({0.5f, 0.0f}, {0.5f, 0.0f});
					rect->SetPivot({0.5f, 0.5f});
					rect->SetOffsetMin({-56.0f, 37.0f});
					rect->SetOffsetMax({56.0f, 39.0f});
				}
			);

			scene->CreateComponentWithInit<ImageUIComponent>(
				obj->GetHandle(),
				[](ImageUIComponent* image)
				{
					image->SetNormalColor({0.85f, 0.72f, 0.32f, 0.9f});
					image->SetOrder(99995);
				}
			);
		}
	);

	scene->ReserveGameObject(
		"RedTeamScoreText", std::nullopt,
		[this, scene](GameObject* obj)
		{
			scene->CreateComponentWithInit<RectTransformComponent>(
				obj->GetHandle(),
				[](RectTransformComponent* rect)
				{
					rect->SetAnchors({0.5f, 0.0f}, {0.5f, 0.0f});
					rect->SetPivot({0.5f, 0.0f});
					rect->SetOffsetMin({72.0f, 16.0f});
					rect->SetOffsetMax({344.0f, 60.0f});
				}
			);

			m_redTeamScoreTextHandle = scene->CreateComponentWithInit<TextUIComponent>(
				obj->GetHandle(),
				[this](TextUIComponent* text)
				{
					text->SetText(FormatRedScore(m_redScore));
					text->SetFontSize(23.0f);
					text->SetHorizontalAlign(TextHorizontalAlign::Center);
					text->SetVerticalAlign(TextVerticalAlign::Center);
					text->SetColor({1.0f, 0.35f, 0.35f, 1.0f});
					text->SetOrder(99995);
				}
			);
		}
	);
}

bool WorldSceneControllerComponent::RefreshTeamScoreText()
{
	auto* scene = GetGameObject()->GetScene();
	if (!scene)
	{
		return false;
	}

	auto* textStorage = scene->GetStorage<TextUIComponent>();
	if (!textStorage)
	{
		return false;
	}

	auto* blueText = textStorage->Get(m_blueTeamScoreTextHandle);
	auto* redText = textStorage->Get(m_redTeamScoreTextHandle);
	if (!blueText || !redText)
	{
		return false;
	}

	blueText->SetText(FormatBlueScore(m_blueScore));
	redText->SetText(FormatRedScore(m_redScore));
	return true;
}
