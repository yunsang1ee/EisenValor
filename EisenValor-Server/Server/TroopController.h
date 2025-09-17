#pragma once

#include "Component.h"

namespace Server {
	namespace Contents {
		class NPC;
		
		// 장수에 붙은 병사들을 컨트롤 하는 컴포넌트
		class TroopController : public Component {
		public:
			enum class FORMATION_TYPE {
				FORMATION_NONE,
				FORMATION_ATTACK,
				FORMATION_DEFENSE,

				END
			};
			
		private:
			std::map<uint32, std::weak_ptr<NPC>>	m_soldiers;
			FORMATION_TYPE							m_form;

		public:
			virtual void Update(const float dt) override;

		public:
			void SetFormation(const FORMATION_TYPE form) noexcept;
			FORMATION_TYPE GetFormation() const noexcept { return m_form; }
			void AddSoldier(std::weak_ptr<NPC> soldier);
			void RemoveSoldier(std::weak_ptr<NPC> soldier);
			const auto& GetSoldiers() const noexcept { return m_soldiers; }

		};
	}
}