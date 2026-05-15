#pragma once

namespace GameServer {
	namespace Contents {
		class BehaviorTree;
		class General;

		enum class BEHAVIOR_NODE_STATUS {
			SUCCESS,
			FAIL,
			RUNNING,
		};

		class BehaviorNode {
		public:
			virtual ~BehaviorNode() = default;

		public:
			virtual void SetOwner(std::shared_ptr<General> const owner) { m_owner = owner; }
			virtual BEHAVIOR_NODE_STATUS Execute(const float dt) abstract;
			virtual void Reset() {}
		
		public:
			std::shared_ptr<General> GetOwner() const { return m_owner.lock(); }

		protected:
			std::weak_ptr<General> m_owner;
		};

		// 여러 자식을 가지고 있는 노드
		class CompositeNode : public BehaviorNode {
		public:
			virtual void Reset() override;
			virtual void SetOwner(std::shared_ptr<General> const owner) override;
		
		public:
			void AddChild(std::unique_ptr<BehaviorNode> child);
		
		protected:
			std::vector<std::unique_ptr<BehaviorNode>> m_children;
		};

		// 모든 자식이 순서대로 성공해야 전체가 성공	
		class SequenceNode : public CompositeNode {
		public:
			virtual BEHAVIOR_NODE_STATUS Execute(const float dt) override;
			virtual void Reset() override { m_currentIndex = 0; CompositeNode::Reset(); }

		private:
			size_t m_currentIndex = 0;
		};

		// 자식 중 하나라도 성공하면 전체가 성공
		class SelectorNode : public CompositeNode {
		public:
			virtual BEHAVIOR_NODE_STATUS Execute(const float dt) override;
			virtual void Reset() override { m_currentIndex = 0; CompositeNode::Reset(); }
		
		private:
			size_t m_currentIndex = 0;
		};


		// 자식이 오직 하나인 노드
		class DecoratorNode : public BehaviorNode {
		public:
			virtual void SetOwner(std::shared_ptr<General> const owner) override;
			virtual void Reset() override;
		
		public:
			void SetChild(std::unique_ptr<BehaviorNode> child);
		
		protected:
			std::unique_ptr<BehaviorNode> m_child;
		};

		// 자식의 결과를 반전시키는 데코레이터 (SUCCESS↔FAIL, RUNNING은 유지)
		class InverterNode : public DecoratorNode {
		public:
			virtual BEHAVIOR_NODE_STATUS Execute(const float dt) override final;
		};

		// 자식을 SUCCESS가 될 때까지 한 번만 실행하고, 이후엔 자식을 호출하지 않고 SUCCESS를 반환한다.
		// Reset() 호출 시 다시 실행 가능 상태로 돌아간다.
		class OnceNode : public DecoratorNode {
		public:
			virtual BEHAVIOR_NODE_STATUS Execute(const float dt) override final;
			virtual void Reset() override;

		private:
			bool m_done{ false };
		};

		class ConditionNode : public BehaviorNode {
		public:
			virtual void Reset() override {}
			virtual bool Check(const float dt) abstract;
			virtual BEHAVIOR_NODE_STATUS Execute(const float dt) override final { return Check(dt) ? BEHAVIOR_NODE_STATUS::SUCCESS : BEHAVIOR_NODE_STATUS::FAIL; }
		};

		class ActionNode : public BehaviorNode {
		public:
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) abstract;
			virtual BEHAVIOR_NODE_STATUS Execute(const float dt) override { return DoAction(dt); }
		};

	}
}


