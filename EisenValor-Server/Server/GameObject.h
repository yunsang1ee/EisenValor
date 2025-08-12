#pragma once
#include "TaskQueue.h"

namespace Server {
	namespace Contents {
		class GameRoom;

		class GameObject {
		private:
			std::wstring				m_name;
			uint32						m_id;
			GAME_OBJECT_TYPE			m_type;

		protected:
			KinematicInfo				m_kinematicInfo;
			
		public:
			std::weak_ptr<GameRoom>		m_room;

		public:
			explicit GameObject(const GAME_OBJECT_TYPE type);
			virtual ~GameObject() = default;

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
			GAME_OBJECT_TYPE GetType() const noexcept { return m_type; }
			const KinematicInfo& GetKinematicInfo() const noexcept { return m_kinematicInfo; }
			const Vec3& GetPos() const noexcept { return m_kinematicInfo.position; }
			const Vec3& GetRotation() const noexcept { return m_kinematicInfo.rotation; }
			const Vec3& GetVelocity() const noexcept { return m_kinematicInfo.velocity; }
			const Vec3& GetAcceleration() const noexcept { return m_kinematicInfo.acceleration; }
			const uint64 GetTimeStamp() const noexcept { return m_kinematicInfo.timeStamp; }

			std::shared_ptr<GameRoom> GetGameWorld() const noexcept { return m_room.lock(); }

		public:
			virtual void Update(const float dt) {}

		};
	}

}


