#pragma once

#include "Component.h"
namespace GameServer {
	namespace Contents {
		class BehaviorNode;

		using BlackboardValue = std::variant<uint32, uint64, float, bool, Vec3>;

        class Blackboard {
        public:
            void SetValue(const std::string_view key, const BlackboardValue& value) { m_data[key.data()] = value; }

            template <typename T>
            // 없으면 defaultValue 넘겨준 값으로...
            T GetValue(const std::string_view key, T defaultValue = T()) const
            {
                auto iter = m_data.find(key.data());
                if(iter != m_data.end()) {
                    return std::get<T>(iter->second);
                }
                return defaultValue;
            }

            bool HasKey(const std::string_view key) const { return m_data.find(key.data()) != m_data.end(); }

            void Clear() { m_data.clear(); }

            void Erase(const std::string_view key) { if(HasKey(key)) m_data.erase(key.data()); }

        private:
            std::map<std::string, BlackboardValue> m_data;
        };

		class BehaviorTree : public Component {
        public:
			BehaviorTree();
            virtual ~BehaviorTree()=default;

		public:
            void        SetRoot(BehaviorNode* const root);
            Blackboard* GetBlackboard() { return &m_blackboard; }

		public:
			virtual void Update(const float dt) override;
			void Reset();

        private:
            BehaviorNode*                   m_root;
            Blackboard                      m_blackboard;

		};
	}
}