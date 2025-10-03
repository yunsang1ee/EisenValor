#pragma once

namespace Server{
	namespace Contents {
		class TroopController;
		
		struct SoldierData {
			Vec3					offsetFromCenter;
			std::shared_ptr<NPC>	soldier;
		};

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
			TroopController* m_controller;
			TROOP_FORMATION_TYPE								m_formationType;
			TROOP_STATE_TYPE									m_stateType;
			Vec3												m_centerPos;			// 대열 중심 위치

		public:
			virtual void Init() abstract;
			virtual void Arrange(std::unordered_map<uint32, SoldierData>& soldiers) noexcept abstract;
		};

		class LineFormation : public IFormation {
		public:
			virtual void Init() override;
			virtual void Arrange(std::unordered_map<uint32, SoldierData>& soldiers) noexcept override;
		};

		class VShapeFormation : public IFormation {
		public:
			virtual void Init() override;
			virtual void Arrange(std::unordered_map<uint32, SoldierData>& soldiers) noexcept override;
		};

		class CircleFormation : public IFormation {
		public:
			virtual void Init() override;
			virtual void Arrange(std::unordered_map<uint32, SoldierData>& soldiers) noexcept override;
		};
	}
}


