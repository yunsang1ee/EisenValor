#pragma once

#include "Component.h"
#include "Formation.h"

namespace Server {
	namespace Contents {
		class NPC;
		class TroopController;

		class TroopController : public Component {
		private:
			std::map<TROOP_FORMATION_TYPE, std::unique_ptr<IFormation>> m_formations;
			std::unordered_map<uint32, SoldierData>						m_soldiers;
			IFormation*													m_curFormation{ nullptr };
			Vec3														m_prevOwnerPos;
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
			IFormation* GetCurFormation() const noexcept { return m_curFormation; }

		public:
			void Arrange();

		};

	}
}
