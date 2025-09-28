#pragma once

#include "Component.h"

namespace Server {
	namespace Contents {
		class NPC;
		class TroopController;

		struct SoldierData {
			Vec3 offset;
			std::shared_ptr<NPC> soldier;
		};

		enum class TROOP_FORMATION_TYPE : uint8 {
			NONE,
			LINE,
			VSHAPE,
			CIRCLE,

			END
		};

		enum class TROOP_STATE_TYPE : uint8 {
			IDLE,
			ATTACK
		};

		class IFormation {
		public:
			TroopController*									m_controller;
			TROOP_FORMATION_TYPE								m_formationType;
			TROOP_STATE_TYPE									m_stateType;
			Vec3												m_centerPos;			// 대열 중심 위치
			virtual void Arrange(std::unordered_map<uint32, SoldierData>& soldiers) noexcept abstract;
		};

		class LineFormation : public IFormation {
		public:
			virtual void Arrange(std::unordered_map<uint32, SoldierData>& soldiers) noexcept;
		};

		// 병사들을 관리하는 컴포넌트
		class TroopController : public Component {
		private:
			IFormation*													m_curFormation{ nullptr };
			std::map<TROOP_FORMATION_TYPE, std::unique_ptr<IFormation>> m_formations;
			std::unordered_map<uint32, SoldierData>						m_soldiers;
		
		public:
			void Init();

		public:
			void AddSoldier(std::shared_ptr<NPC> soldier);
			void RemoveSoldier(std::shared_ptr<NPC> soldier);

		public:
			virtual void Update(const float dt) override;
			
		public:
			void SetTargetPos(const Vec3& targetPos);
			void SetFormation(const TROOP_FORMATION_TYPE type) noexcept;

		};

	}
}
