#pragma once
#include "TaskQueue.h"
#include "FSM.h"
#include "Script.h"

struct GameObjectData;

namespace Server {
	namespace Contents {
		class GameRoom;
		class Component;
		class FSM;
		class BehaviorTree;
		class NavAgent;
		class Script;
		class GameWorld;
		class Collider;
		class OBBCollider;

		class GameObject {
		private:
			using ComponentGroup = std::array<std::unique_ptr<Component>, etou8(COMPONENT_TYPE::END)>;
			using Scripts = std::vector<std::unique_ptr<Script>>;
			
			std::wstring							m_name;
			uint32									m_id;
			const FB_ENUMS::GAME_OBJECT_TYPE		m_type;
			const FB_ENUMS::TEAM_TYPE				m_teamType;
			
			ComponentGroup							m_components;
			Scripts									m_scripts;

			std::weak_ptr<GameRoom>					m_room;
			std::weak_ptr<GameWorld>				m_gameWorld;

			PosInfo									m_posInfo;
			Vec3									m_scale;

			bool									m_isCreature;
			const GameObjectData*					m_gameObjectData;
			bool									m_active;
		
		public:
			GameObject() = default;
			explicit GameObject(const FB_ENUMS::TEAM_TYPE teamType, const FB_ENUMS::GAME_OBJECT_TYPE type);
			virtual ~GameObject();
			GameObject(const GameObject&) = delete;
			GameObject& operator=(const GameObject&) = delete;
			GameObject (GameObject&&) = default;
			GameObject& operator=(GameObject&&) = default;


		public:
			virtual void OnCollisionEnter(Collider* const other) {}
			virtual void OnCollisionStay(Collider* const other) {}
			virtual void OnCollisionExit(Collider* const other) {}

			virtual void Update(const float dt);

		public:
			template<std::derived_from<Component> T>
			T* GetComponent()
			{
				if constexpr(std::is_same_v<FSM, T>) {
					return static_cast<FSM*>(m_components[etou8(COMPONENT_TYPE::FSM)].get());
				}
				else if constexpr(std::is_same_v<BehaviorTree, T>) {
					return static_cast<BehaviorTree*>(m_components[etou8(COMPONENT_TYPE::BEHAVIOR_TREE)].get());
				}
				else if constexpr(std::is_same_v<NavAgent, T>) {
					return static_cast<NavAgent*>(m_components[etou8(COMPONENT_TYPE::NAV_AGENT)].get());
				}
				else if constexpr(std::is_same_v<Collider, T>) {
					return static_cast<Collider*>(m_components[etou8(COMPONENT_TYPE::COLLIDER)].get());
				}

				return nullptr;
			}

			template<std::derived_from<Component> T, typename... Args>
			auto AddComponent(Args&&... args)
			{
				auto component = std::make_unique<T>(std::forward<Args>(args)...);
				component->SetOwner(this);

				if constexpr(std::is_same_v<FSM, T>) {
					m_components[etou8(COMPONENT_TYPE::FSM)] = std::move(component);
				}
				else if constexpr(std::is_same_v<BehaviorTree, T>) {
					m_components[etou8(COMPONENT_TYPE::BEHAVIOR_TREE)] = std::move(component);
				}
				else if constexpr(std::is_same_v<NavAgent, T>) {
					m_components[etou8(COMPONENT_TYPE::NAV_AGENT)] = std::move(component);
				}
				else if constexpr(std::derived_from<T, Collider>) {
					m_components[etou8(COMPONENT_TYPE::COLLIDER)] = std::move(component);
				}
				
				return GetComponent<T>();
			}

			template<std::derived_from<Script> T>
			Script* AddScript(std::unique_ptr<T> script)
			{
				Script* s = script.get();
				m_scripts.emplace_back(std::move(script));
				return s;
			}

		public:
			void SetID(const uint32 id) noexcept { m_id = id; }
			void SetName(std::wstring_view name) { m_name = name.data(); }
			void SetPosInfo(const PosInfo& transform) noexcept { m_posInfo = transform; }
			void SetPos(const Vec3& pos) noexcept { m_posInfo.pos = pos; }
			void SetRotation(const Vec3& rotation) noexcept { m_posInfo.rot = rotation; }
			void SetRoom(std::weak_ptr<GameRoom> match) noexcept { m_room = match; }
			void SetGameWorld(std::shared_ptr<GameWorld> gameWorld) noexcept { m_gameWorld = gameWorld; }
			void SetCreature(bool flag) { m_isCreature = flag; }
			void SetGameObjectData(const GameObjectData* const data) { m_gameObjectData = data; }
			void SetActive(const bool active) noexcept { m_active = active; }

			const std::wstring& GetName() const noexcept { return m_name; }
			uint32 GetID() const noexcept { return m_id; }
			FB_ENUMS::GAME_OBJECT_TYPE GetObjType() const noexcept { return m_type; }
			const PosInfo& GetPosInfo() const noexcept { return m_posInfo; }
			const Vec3& GetPos() const noexcept { return m_posInfo.pos; }
			const Vec3& GetRotation() const noexcept { return m_posInfo.rot; }
			const Vec3& GetScale() const noexcept { return m_scale; }
			std::shared_ptr<GameRoom> GetGameRoom() const noexcept { return m_room.lock(); }
			FB_ENUMS::TEAM_TYPE GetTeamType() const noexcept { return m_teamType; }
			Vec3 GetForwardDir();
			std::shared_ptr<GameWorld> GetGameWorld() { return m_gameWorld.lock(); }
			bool IsCreature() const noexcept { return m_isCreature; }
			bool IsActive() { return m_active; }
			const GameObjectData* GetGameObjectData() const { return m_gameObjectData; }
		};
	}
}