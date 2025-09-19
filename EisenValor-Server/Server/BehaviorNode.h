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
			std::weak_ptr<BehaviorTree> m_tree;

		public:
			virtual ~BehaviorNode() = default;

		public:
			virtual void SetTree(std::shared_ptr<BehaviorTree> tree) { m_tree = tree; }
			std::shared_ptr<BehaviorTree> GetTree() const { return m_tree.lock(); }
		public:
			virtual BEHAVIOR_NODE_STATUS Execute(const float dt) abstract;
			virtual void Reset() {}
		};

		// 여러 자식을 가지고 있는 노드
		class CompositeNode : public BehaviorNode {
		protected:
			std::vector<std::unique_ptr<BehaviorNode>> m_children;

		public:
			void AddChild(std::unique_ptr<BehaviorNode> child)
			{
				m_children.emplace_back(std::move(child));
				if(auto tree = m_tree.lock()) {
					// 트리가 이미 세팅된 상태라면 자식에게도 전달
					m_children.back()->SetTree(tree);
				}
			}

			virtual void SetTree(std::shared_ptr<BehaviorTree> tree) override
			{
				BehaviorNode::SetTree(tree);
				for(auto& child : m_children) {
					child->SetTree(tree);
				}

			}
			void Reset() override
			{
				for(auto& child : m_children) {
					child->Reset();
				}
			}

		};

		// 자식이 오직 하나인 노드
		class DecoratorNode : public BehaviorNode {
		protected:
			std::unique_ptr<BehaviorNode> m_child;

		public:
			void SetChild(std::unique_ptr<BehaviorNode> child)
			{
				m_child = std::move(child);
				if(auto tree = m_tree.lock()) {
					m_child->SetTree(tree);
				}
			}

			void SetTree(std::shared_ptr<BehaviorTree> tree) override
			{
				BehaviorNode::SetTree(tree);
				if(m_child) {
					m_child->SetTree(tree);
				}
			}

			void Reset() override
			{
				if(m_child) {
					m_child->Reset();
				}
			}
		};

		class ConditionNode : public BehaviorNode {
		public:
			virtual bool Check() abstract;

		public:
			virtual BEHAVIOR_NODE_STATUS Execute(const float dt) override
			{
				return Check() ? BEHAVIOR_NODE_STATUS::SUCCESS : BEHAVIOR_NODE_STATUS::FAIL;
			}
		};

		class ActionNode : public BehaviorNode {
		public:
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) abstract;

			BEHAVIOR_NODE_STATUS Execute(const float dt) override { return DoAction(dt); }
		};

		// 모든 자식이 순서대로 성공해야 전체가 성공	
		class SequenceNode : public CompositeNode {
		private:
			size_t m_currentIndex = 0;

		public:
			BEHAVIOR_NODE_STATUS Execute(float DeltaTime) override
			{
				for(; m_currentIndex < m_children.size(); ++m_currentIndex) {
					auto& child = m_children[m_currentIndex];
					auto status = child->Execute(DeltaTime);

					if(status == BEHAVIOR_NODE_STATUS::RUNNING) 
						return BEHAVIOR_NODE_STATUS::RUNNING;
					if(status == BEHAVIOR_NODE_STATUS::FAIL) {
						m_currentIndex = 0;
						return BEHAVIOR_NODE_STATUS::FAIL;
					}
				}
				m_currentIndex = 0;
				return BEHAVIOR_NODE_STATUS::SUCCESS;
			}

			void Reset() override
			{
				m_currentIndex = 0;
				CompositeNode::Reset();
			}

		};

		// 자식 중 하나라도 성공하면 전체가 성공
		class SelectorNode : public CompositeNode {
		private:
			size_t m_currentIndex = 0;

		public:
			BEHAVIOR_NODE_STATUS Execute(float DeltaTime) override
			{
				for(; m_currentIndex < m_children.size(); ++m_currentIndex) {
					auto& child = m_children[m_currentIndex];
					auto status = child->Execute(DeltaTime);

					if(status == BEHAVIOR_NODE_STATUS::RUNNING) 
						return BEHAVIOR_NODE_STATUS::RUNNING;
					if(status == BEHAVIOR_NODE_STATUS::SUCCESS) {
						m_currentIndex = 0;
						return BEHAVIOR_NODE_STATUS::SUCCESS;
					}
				}
				m_currentIndex = 0;
				return BEHAVIOR_NODE_STATUS::FAIL;
			}

			void Reset() override
			{
				m_currentIndex = 0;
				CompositeNode::Reset();
			}
		};

	
	}
}


