#pragma once
#include "TaskQueue.h"
#include "FSM.h"
#include "Script.h"
#include "Transform.h"

struct GameObjectData;

namespace GameServer {
	namespace Contents {
		class GameRoom;
		class Component;
		class Movement;
		class FSM;
		class BehaviorTree;
		class NavAgent;
		class Script;
		class GameWorld;
		class GameWorld;
		class Collider;
		class OBBCollider;

		class GameObject : public std::enable_shared_from_this<GameObject> {
		private:
			using ComponentGroup = std::array<std::unique_ptr<Component>, etou8(COMPONENT_TYPE::END)>;
			using Scripts = std::vector<std::unique_ptr<Script>>;
		
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
				if constexpr(std::derived_from<T, Movement>) {
					return static_cast<Movement*>(m_components[etou8(COMPONENT_TYPE::MOVEMENT)].get());
				}
				else if constexpr(std::is_same_v<FSM, T>) {
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
				component->SetOwner(shared_from_this());

				if constexpr(std::derived_from<T, Movement>) {
					m_components[etou8(COMPONENT_TYPE::MOVEMENT)] = std::move(component);
				}
				else if constexpr(std::is_same_v<FSM, T>) {
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

			Script* GetScript(const std::string_view name);

		public:
			void SetID(const uint64 id)  { m_id = id; }
			void SetName(std::wstring_view name) { m_name = name.data(); }
			void SetCreature(bool flag) { m_isCreature = flag; }
			void SetGameObjectData(const GameObjectData* const data) { m_gameObjectData = data; }
			void SetActive(const bool active) { m_active = active; }
			void SetTransform(const Transform& transform) { m_transform = transform; }
			const std::wstring& GetName() const { return m_name; }
			uint64 GetID() const { return m_id; }
			FB_ENUMS::GAME_OBJECT_TYPE GetObjType() const { return m_objType; }

			void        SetPosition(const Vec3& pos) { m_transform.SetPosition(pos); }
			const Vec3& GetPosition() const { return m_transform.GetPosition(); }

			void        SetRotation(const Vec3& rotDegree) { m_transform.SetRotation(rotDegree); }
			const Vec3& GetRotation() const { return m_transform.GetRotation(); }  // Radian
			Vec3        GetRotationDegree() const { return m_transform.GetRotationDegree(); }

			Transform& GetTransform() { return m_transform; }
			const Transform& GetTransform() const { return m_transform; }

			Vec3 GetForward() const { return m_transform.GetForward(); }
			Vec3 GetRight()   const { return m_transform.GetRight(); }
			Vec3 GetBack()    const { return m_transform.GetBack(); }
			Vec3 GetLeft()    const { return m_transform.GetLeft(); }

			void LookAt(const Vec3& target) { m_transform.LookAt(target); }

			const Vec3& GetScale() const  { return m_scale; }

			void SetGameWorld(GameWorld* const gameWorld) { m_gameWorld = gameWorld; }
			GameWorld* GetGameWorld() const { return m_gameWorld; }

			FB_ENUMS::TEAM_TYPE GetTeamType() const  { return m_teamType; }
			bool IsCreature() const  { return m_isCreature; }
			bool IsActive() const { return m_active; }
			const GameObjectData* GetGameObjectData() const { return m_gameObjectData; }
			
			bool IsTargetInRange(std::shared_ptr<GameObject> const target, const float rangeSq = 2.f * 2.f);
			bool IsSameTeam(std::shared_ptr<GameObject> const other);

		private:
			std::wstring							m_name;
			uint64									m_id;
			const FB_ENUMS::GAME_OBJECT_TYPE		m_objType;
			const FB_ENUMS::TEAM_TYPE				m_teamType;

			ComponentGroup							m_components;
			Scripts									m_scripts;

			GameWorld*								m_gameWorld;

			Vec3									m_scale;

			Transform								m_transform;

			bool									m_isCreature;
			const GameObjectData*					m_gameObjectData;
			bool									m_active;
		};
	}
}