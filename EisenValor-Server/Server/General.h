#pragma once
#include "Creature.h"

namespace Server {
	namespace Contents {
		class General : public Creature {
		private:
			FB_ENUMS::GENERAL_STANCE_TYPE m_stanceType;

		public:
			General(const FB_ENUMS::TEAM_TYPE teamType, const FB_ENUMS::GAME_OBJECT_TYPE objType = FB_ENUMS::GAME_OBJECT_TYPE_GENERAL);
			virtual ~General() = default;

		public:
			void SetStanceType(const FB_ENUMS::GENERAL_STANCE_TYPE stanceType) noexcept { m_stanceType = stanceType; }
			FB_ENUMS::GENERAL_STANCE_TYPE GetStanceType() const noexcept { return m_stanceType; }
		};
	}
}

