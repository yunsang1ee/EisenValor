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
		class GameWorldTest;
		class Collider;
		class OBBCollider;

		class GameObject {
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
			void LookAt(const Vec3& lookAt, const float dt);

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

			Script* GetScript(const std::string_view name);

		public:
			void SetID(const uint32 id)  { m_id = id; }
			void SetName(std::wstring_view name) { m_name = name.data(); }
			void SetPosInfo(const PosInfo& transform) { m_posInfo = transform; }
			void SetPos(const Vec3& pos) { m_posInfo.pos = pos; }
			void SetRotation(const Vec3& rotation) { m_posInfo.rot = rotation; }
			void SetLook(const Vec3& look) { m_look = look; }
#ifdef LEGACY_CODE
			void SetRoom(std::weak_ptr<GameRoom> match) { m_room = match; }
			void SetGameWorld(std::shared_ptr<GameWorld> gameWorld) { m_gameWorld = gameWorld; }
#endif
			void SetCreature(bool flag) { m_isCreature = flag; }
			void SetGameObjectData(const GameObjectData* const data) { m_gameObjectData = data; }
			void SetActive(const bool active) { m_active = active; }
			void SetRotateSpeed(const float rotateSpeed) { m_rotateSpeed = rotateSpeed; }

			const std::wstring& GetName() const { return m_name; }
			uint32 GetID() const { return m_id; }
			FB_ENUMS::GAME_OBJECT_TYPE GetObjType() const { return m_type; }
			const PosInfo& GetPosInfo() const { return m_posInfo; }
			const Vec3& GetPos() const  { return m_posInfo.pos; }
			const Vec3& GetRotation() const  { return m_posInfo.rot; }
			const Vec3& GetScale() const  { return m_scale; }
			const Vec3& GetLook() const { return m_look; }
#ifdef LEGACY_CODE
			std::shared_ptr<GameRoom> GetGameRoom() const  { return m_room.lock(); }
			std::shared_ptr<GameWorld> GetGameWorld() { return m_gameWorld.lock(); }
#endif

#ifdef MODERN_CODE
			void SetGameWorld(GameWorldTest* const gameWorld) { m_gameWorld = gameWorld; }
			GameWorldTest* GetGameWorld() const { return m_gameWorld; }
#endif

			FB_ENUMS::TEAM_TYPE GetTeamType() const  { return m_teamType; }
			Vec3 GetForwardDir();
			bool IsCreature() const  { return m_isCreature; }
			bool IsActive() const { return m_active; }
			const GameObjectData* GetGameObjectData() const { return m_gameObjectData; }
			
			bool IsTargetInRange(const GameObject* const target, const float rangeSq = 2.f * 2.f);
			bool IsSameTeam(const GameObject* const other);

		private:
			std::wstring							m_name;
			uint32									m_id;
			const FB_ENUMS::GAME_OBJECT_TYPE		m_type;
			const FB_ENUMS::TEAM_TYPE				m_teamType;

			ComponentGroup							m_components;
			Scripts									m_scripts;

#ifdef LEGACY_CODE
			std::weak_ptr<GameRoom>					m_room;
			std::weak_ptr<GameWorld>				m_gameWorld;
#endif

#ifdef MODERN_CODE
			GameWorldTest*							m_gameWorld;
#endif

			PosInfo									m_posInfo;
			Vec3									m_scale;
			Vec3									m_look;

			float									m_rotateSpeed;

			bool									m_isCreature;
			const GameObjectData*					m_gameObjectData;
			bool									m_active;
		};
	}
}