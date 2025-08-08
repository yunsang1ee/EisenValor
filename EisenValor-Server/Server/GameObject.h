#pragma once
#include "TaskQueue.h"

namespace Server {
	namespace Contents {
		class GameWorld;

		class GameObject : public ServerEngine::TaskQueue {
		private:
			std::wstring				m_name;
			uint32						m_id;
			GAME_OBJECT_TYPE			m_type;

			KinematicInfo				m_kinematicInfo;
			
			std::weak_ptr<GameWorld>	m_match;

		public:
			explicit GameObject(const GAME_OBJECT_TYPE type);
			virtual ~GameObject() = default;

		public:
			void SetID(const uint32 id) noexcept { m_id = id; }
			void SetName(std::wstring_view name) { m_name = name.data(); }
			void SetKinematicInfo(const KinematicInfo& transform) noexcept { m_kinematicInfo = transform; }
			void SetPos(const Vec3& pos) noexcept { m_kinematicInfo.position = pos; }
			void SetRotation(const Vec3& rotation) noexcept { m_kinematicInfo.rotation = rotation; }

			void SetWorld(std::weak_ptr<GameWorld> match) noexcept { m_match = match; }

			const std::wstring& GetName() const noexcept { return m_name; }
			uint32 GetID() const noexcept { return m_id; }
			GAME_OBJECT_TYPE GetType() const noexcept { return m_type; }
			const KinematicInfo& GetKinematicInfo() const noexcept { return m_kinematicInfo; }
			const Vec3& GetPos() const noexcept { return m_kinematicInfo.position; }
			const Vec3& GetRotation() const noexcept { return m_kinematicInfo.rotation; }
			std::shared_ptr<GameWorld> GetGameWorld() const noexcept { return m_match.lock(); }

		};
	}

}


