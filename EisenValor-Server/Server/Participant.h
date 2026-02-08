#pragma once

namespace Server {

	namespace Contents {
		class Participant {
		private:
			ParticipantInfo	m_info;

		public:
			explicit Participant(const uint32 id, const FB_ENUMS::PARTICIPANT_TYPE type, const FB_ENUMS::TEAM_TYPE teamType);

		public:
			void SetTeamType(const FB_ENUMS::TEAM_TYPE teamType) { m_info.teamType = teamType; }
			void SetStateType(const FB_ENUMS::PARTICIPANT_STATE_TYPE stateType) { m_info.stateType = stateType; }
			uint32 GetID() const noexcept { return m_info.id; }
			FB_ENUMS::TEAM_TYPE GetTeamType() const noexcept { return m_info.teamType; }
			FB_ENUMS::PARTICIPANT_TYPE GetType() const noexcept { return m_info.type; }
			FB_ENUMS::PARTICIPANT_STATE_TYPE GetStateType() const noexcept { return m_info.stateType; }
			const ParticipantInfo& GetInfo() const noexcept { return m_info; }
		};

		class User : public Participant {
		private:
			std::shared_ptr<ClientSession> m_session;

		public:
			explicit User(const uint32 id, const FB_ENUMS::PARTICIPANT_TYPE type, const FB_ENUMS::TEAM_TYPE teamType, std::shared_ptr<ClientSession> clientSession);

		public:
			std::shared_ptr<ClientSession> GetSession() { return m_session; }

		};

		class Bot : public Participant {
		public:
			explicit Bot(const uint32 id, const FB_ENUMS::TEAM_TYPE teamType);
		};
	}
}