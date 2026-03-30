#pragma once

#include "Component.h"

namespace GameServer {
	namespace Contents {
		class NavSystem;
		class NavAgent : public Component {
		public:
			explicit NavAgent(NavSystem* const navSystem);
			virtual ~NavAgent() = default;

		public:
			bool Init(const dtCrowdAgentParams& params);
			virtual void Update(const float dt) override final;

		public:
			void SetDestPos(const Vec3& destPos);
			void StopMove();
			void Remove();
			
		private:
			NavSystem*			m_navSystem;
			int32				m_agentIdx;
			dtCrowdAgentParams	m_params;

			Vec3				m_destPos;
			bool				m_hasTarget;

		};
	}
}

