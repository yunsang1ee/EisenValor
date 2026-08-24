#pragma once

#include <IComponent.h>
#include "GameObject.h"
#include "ImageUIComponent.h"
#include "Packets/Enums_generated.h"
#include "TextUIComponent.h"
#include <array>
#include <unordered_map>

class OccupationZoneRegistrationComponent final : public ComponentBase<OccupationZoneRegistrationComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "OccupationZoneRegistrationComponent"; }

	void OnStart() override;
};

class WorldSceneControllerComponent final : public ComponentBase<WorldSceneControllerComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "WorldSceneControllerComponent"; }

	void OnStart() override;
	void OnUpdate(float deltaTime);
	void SetTeamScores(uint8_t blueScore, uint8_t redScore);
	void RegisterOccupationZone(uint64 zoneID, const Vec3& position);
	void SetOccupationZoneOwner(uint64 zoneID, FB_ENUMS::TEAM_TYPE ownerTeam);
	void SetOccupationZoneGauge(uint64 zoneID, float gauge, FB_ENUMS::TEAM_TYPE dominantTeam);

private:
	static constexpr size_t kOccupationZoneCount = 2;
	// TODO(packet): Replace this client estimate with the server's grace_remaining_ms snapshot.
	static constexpr float kRecaptureGraceSeconds = 5.0f;

	struct OccupationZoneState
	{
		Vec3				position{};
		FB_ENUMS::TEAM_TYPE ownerTeam = FB_ENUMS::TEAM_TYPE_NONE;
		FB_ENUMS::TEAM_TYPE dominantTeam = FB_ENUMS::TEAM_TYPE_NONE;
		float				gauge = 0.0f;
		float				graceRemaining = 0.0f;
		int					displayedGraceTenths = -1;
		bool				registered = false;
		bool				hasGauge = false;
		bool				graceActive = false;
		bool				markerGaugeDirty = true;
	};

	struct OccupationZoneUI
	{
		HandleOf<GameObject>	   markerRootHandle;
		HandleOf<ImageUIComponent> markerBadgeHandle;
		HandleOf<ImageUIComponent> markerBadgeInnerHandle;
		std::array<HandleOf<ImageUIComponent>, 4> markerGaugeSegmentHandles;
		HandleOf<TextUIComponent>  markerLabelHandle;
		HandleOf<ImageUIComponent> gaugeBlueFillHandle;
		HandleOf<ImageUIComponent> gaugeRedFillHandle;
		HandleOf<ImageUIComponent> gaugeLabelBackgroundHandle;
		HandleOf<TextUIComponent>  gaugeLabelHandle;
		HandleOf<GameObject>	   graceRootHandle;
		HandleOf<TextUIComponent>  graceTextHandle;
	};

	void   CreateTeamScoreUI();
	void   CreateOccupationZoneUI();
	void   CreateOccupationZoneMarker(size_t slot);
	void   CreateOccupationZoneGraceText(size_t slot);
	void   ResolveOccupationZoneOrder();
	void   BindOccupationGaugeUI();
	void   UpdateOccupationZoneUI(float deltaTime);
	void   UpdateOccupationGauge(size_t slot);
	void   UpdateOccupationMarker(size_t slot);
	void   UpdateOccupationMarkerGauge(size_t slot);
	void   UpdateOccupationOwnerStyle(size_t slot);
	size_t FindOccupationZoneSlot(uint64 zoneID) const;
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	void CreateRestirDebugOverlayUI();
#endif
	bool RefreshTeamScoreText();
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	bool RefreshRestirDebugOverlay();
#endif

	HandleOf<TextUIComponent>						   m_blueTeamScoreTextHandle;
	HandleOf<TextUIComponent>						   m_redTeamScoreTextHandle;
	std::unordered_map<uint64, OccupationZoneState>	   m_occupationZones;
	std::array<uint64, kOccupationZoneCount>		   m_occupationZoneIDs{};
	std::array<OccupationZoneUI, kOccupationZoneCount> m_occupationZoneUI{};
	bool											   m_occupationZoneOrderReady = false;
	bool											   m_occupationGaugeUIBound = false;
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	HandleOf<TextUIComponent> m_restirDebugTextHandle;
	uint64					  m_lastRestirDebugRevision = ~uint64{0};
#endif
	uint8_t m_blueScore = 0;
	uint8_t m_redScore = 0;
	bool	m_scoreTextDirty = true;
};
