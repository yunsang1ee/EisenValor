#pragma once
#include "TaskQueue.h"
#include "FSM.h"
#include "Script.h"

namespace Server {
	namespace Contents {
		class GameRoom;
		class Component;
		class FSM;
		class BehaviorTree;
		class Script;
		class TroopController;

		class GameObject {
		private:
			std::wstring							m_name;
			uint32									m_id;
			const GAME_OBJECT_TYPE					m_type;
			std::array<std::unique_ptr<Component>, etou8(COMPONENT_TYPE::END)> m_components;
			std::vector<std::unique_ptr<Script>>	m_scripts;
			TEAM_TYPE								m_teamType;

		protected:
			KinematicInfo							m_kinematicInfo;
			
		public:
			std::weak_ptr<GameRoom>					m_room;

		public:
			explicit GameObject(const GAME_OBJECT_TYPE type, const TEAM_TYPE teamType);
			virtual ~GameObject();

		public:
			void SetID(const uint32 id) noexcept { m_id = id; }
			void SetName(std::wstring_view name) { m_name = name.data(); }
			void SetKinematicInfo(const KinematicInfo& transform) noexcept { m_kinematicInfo = transform; }
			void SetPos(const Vec3& pos) noexcept { m_kinematicInfo.position = pos; }
			void SetRotation(const Vec3& rotation) noexcept { m_kinematicInfo.rotation = rotation; }
			void SetVelocity(const Vec3& velocity) noexcept { m_kinematicInfo.velocity = velocity; }
			void SetAcceleration(const Vec3& acceleration) noexcept { m_kinematicInfo.acceleration = acceleration; }
			void SetTimeStamp(const uint64 timeStamp) noexcept { m_kinematicInfo.timeStamp = timeStamp; }
			void SetRoom(std::weak_ptr<GameRoom> match) noexcept { m_room = match; }

			const std::wstring& GetName() const noexcept { return m_name; }
			uint32 GetID() const noexcept { return m_id; }
			GAME_OBJECT_TYPE GetObjType() const noexcept { return m_type; }
			const KinematicInfo& GetKinematicInfo() const noexcept { return m_kinematicInfo; }
			const Vec3& GetPos() const noexcept { return m_kinematicInfo.position; }
			const Vec3& GetRotation() const noexcept { return m_kinematicInfo.rotation; }
			const Vec3& GetVelocity() const noexcept { return m_kinematicInfo.velocity; }
			const Vec3& GetAcceleration() const noexcept { return m_kinematicInfo.acceleration; }
			const uint64 GetTimeStamp() const noexcept { return m_kinematicInfo.timeStamp; }
			std::shared_ptr<GameRoom> GetGameRoom() const noexcept { return m_room.lock(); }
			TEAM_TYPE GetTeamType() const noexcept { return m_teamType; }
			const Vec3 GetForward();

		public:
			virtual void Update(const float dt);

		public:
			template<typename T> requires std::derived_from<T, Component>
			T* GetComponent()
			{
				if constexpr(std::is_same_v<FSM, T>) {
					return static_cast<FSM*>(m_components[etou8(COMPONENT_TYPE::FSM)].get());
				}
				else if constexpr(std::is_same_v<BehaviorTree, T>) {
					return static_cast<BehaviorTree*>(m_components[etou8(COMPONENT_TYPE::BEHAVIOR_TREE)].get());
				}
				else if constexpr(std::is_same_v<TroopController, T>) {
					return static_cast<TroopController*>(m_components[etou8(COMPONENT_TYPE::TROOP_CONTROLLER)].get());
				}
			}

			template<typename T> requires std::derived_from<T, Component>
			auto AddComponent()
			{
				if constexpr(std::is_same_v<FSM, T>) {
					m_components[etou8(COMPONENT_TYPE::FSM)] = std::make_unique<T>();
					return GetComponent<T>();
				}
				else if constexpr(std::is_same_v<BehaviorTree, T>) {
					m_components[etou8(COMPONENT_TYPE::BEHAVIOR_TREE)] = std::make_unique<T>();
					return GetComponent<T>();
				}
				else if constexpr(std::is_same_v<TroopController, T>) {
					m_components[etou8(COMPONENT_TYPE::TROOP_CONTROLLER)] = std::make_unique<T>();
					return GetComponent<T>();
				}
			}

			template<typename T> requires std::derived_from<T, Script>
			void AddScript(std::unique_ptr<Script> script) { m_scripts.emplace_back(std::move(script)); }
		};
	}
}