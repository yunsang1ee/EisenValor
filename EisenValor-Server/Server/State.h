#pragma once
#include "Component.h"

#define DECLARE_CREATE_FUNC(StateClass)														\
    friend class GameObjectFactory;															\
    friend struct std::default_delete<Server::Contents::StateClass>;						\
private:																					\
    template <typename... Args>																\
    static std::unique_ptr<StateClass> Create(Args&&... args) {								\
        return std::unique_ptr<StateClass>(new StateClass(std::forward<Args>(args)...));	\
    }

namespace Server {
	namespace Contents {
		class FSM;

		class State {
		private:
			FSM*							m_fsm;
			uint8							m_type;

		protected:
			explicit State(const uint8 type);

		public:
			virtual ~State();

		public:
			virtual void Enter(const float dt) abstract;
			virtual void Exit(const float dt) abstract;

		public:
			virtual void Update(const float dt) abstract;

		public:
			uint8 GetStateType() const noexcept { return m_type; }

		public:
			void SetFSM(FSM* fsm) { m_fsm = fsm; }
			FSM* GetFSM() const { return m_fsm; }
		};
	}
}

