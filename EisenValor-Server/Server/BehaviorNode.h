#pragma once

namespace Server {
	namespace Contents {
		class BehaviorTree;

		enum class BEHAVIOR_NODE_STATUS {
			SUCCESS,
			FAIL,
			RUNNING,
		};

		class BehaviorNode {
		protected:
			BehaviorTree* m_tree;

		public:
			virtual ~BehaviorNode();

		public:
			virtual void SetTree(BehaviorTree* const tree) { m_tree = tree; }
			BehaviorTree* GetTree() const { return m_tree; }
		public:
			virtual BEHAVIOR_NODE_STATUS Execute(const float dt) abstract;
			virtual void Reset() {}
		};

		// 여러 자식을 가지고 있는 노드
		class CompositeNode : public BehaviorNode {
		protected:
			std::vector<std::unique_ptr<BehaviorNode>> m_children;

		public:
			void AddChild(std::unique_ptr<BehaviorNode> child);
			virtual void SetTree(BehaviorTree* const tree) override;
			void Reset() override;

		};

		// 모든 자식이 순서대로 성공해야 전체가 성공	
		class SequenceNode : public CompositeNode {
		private:
			size_t m_currentIndex = 0;

		public:
			BEHAVIOR_NODE_STATUS Execute(const float dt) override;
			void Reset() override;

		};

		// 자식 중 하나라도 성공하면 전체가 성공
		class SelectorNode : public CompositeNode {
		public:
			BEHAVIOR_NODE_STATUS Execute(const float dt) override;

			void Reset() override;
		};


		// 자식이 오직 하나인 노드
		class DecoratorNode : public BehaviorNode {
		protected:
			std::unique_ptr<BehaviorNode> m_child;

		public:
			void SetChild(std::unique_ptr<BehaviorNode> child);
			void SetTree(BehaviorTree* const tree) override;
			void Reset() override;
		};

		class ConditionNode : public BehaviorNode {
		public:
			virtual bool Check(const float dt) abstract;

		public:
			virtual BEHAVIOR_NODE_STATUS Execute(const float dt) override final { return Check(dt) ? BEHAVIOR_NODE_STATUS::SUCCESS : BEHAVIOR_NODE_STATUS::FAIL; }
		};

		class ActionNode : public BehaviorNode {
		public:
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) abstract;
			BEHAVIOR_NODE_STATUS Execute(const float dt) override { return DoAction(dt); }
		};

	}
}


