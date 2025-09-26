#pragma once

#include "Component.h"

namespace Server {
	namespace Contents {
		class NPC;
		class TroopController;

		enum class TROOP_FORMATION_TYPE : uint8 {
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
			virtual void Arrange(std::unordered_map<uint32, std::shared_ptr<NPC>>& soldiers) noexcept abstract;
		};

		class LineFormation : public IFormation {
		public:
			virtual void Arrange(std::unordered_map<uint32, std::shared_ptr<NPC>>& soldiers) noexcept;
		};

		// 병사들을 관리하는 컴포넌트
		class TroopController : public Component {
		private:
			IFormation*													m_curFormation;
			std::map<TROOP_FORMATION_TYPE, std::unique_ptr<IFormation>> m_formations;
			std::unordered_map<uint32, std::shared_ptr<NPC>>			m_soldiers;

		public:
			void Init();

		public:
			void AddSoldier(std::shared_ptr<NPC> soldier);
			void RemoveSoldier(std::shared_ptr<NPC> soldier);

		public:
			virtual void Update(const float dt) override;
			
		public:
			void SetFormation(const TROOP_FORMATION_TYPE type) noexcept;

		};

	}
}
