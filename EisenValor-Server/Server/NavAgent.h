#pragma once

#include "Component.h"

namespace Server {
	namespace Contents {
		class NavSystem;
		class NavAgent : public Component {
		private:
			NavSystem*			m_navSystem;
			int32				m_agentIdx;
			dtCrowdAgentParams	m_params;
			
			Vec3				m_targetPos;
			bool				m_hasTarget;

		public:
			explicit NavAgent(NavSystem* const navSystem);
			virtual ~NavAgent() = default;

		public:
			bool Init(const dtCrowdAgentParams& params);
			virtual void Update(const float dt) override final;

		public:
			void SetTargetPos(const Vec3& targetPos);
			
		};
	}
}

