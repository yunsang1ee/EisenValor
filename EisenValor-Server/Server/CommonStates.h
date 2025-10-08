#pragma once

#include "State.h"

namespace Server {
	namespace Contents {
		class IdleState : public State {
		public:
			int32 detectionEnemyRange{};

		public:
			explicit IdleState(const uint8 type);
			virtual ~IdleState() = default;

		public:

		};

		class RunState : public State {
		public:
			explicit RunState(const uint8 type);
			virtual ~RunState() = default;

		public:
		};

		class AttackState : public State {
		public:
			int32 attackRange;

		public:
			explicit AttackState(const uint8 type);
			virtual ~AttackState() = default;

		public:
		};

		class DefenseState : public State {
		public:
			explicit DefenseState(const uint8 type);
			virtual ~DefenseState() = default;

		public:
		};

		class DeadState : public State {
		public:
			explicit DeadState(const uint8 type);
			virtual ~DeadState() = default;

		public:
		};

	}
}
