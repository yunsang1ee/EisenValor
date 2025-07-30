#pragma once

namespace Server {
	namespace Contents {
		class GameMatch;

		class GameObject {
		private:
			std::wstring	m_name;
			uint32			m_id;	// GENERAL의 경우 SESSION ID와 동일하게 
			Transform		m_transform;

			std::weak_ptr<GameMatch> m_match;

		public:
			explicit GameObject();
			virtual ~GameObject() = default;

		public:
			void SetID(const uint32 id) noexcept { m_id = id; }
			void SetName(std::wstring_view name) { m_name = name.data(); }
			void SetTransform(const Transform& transform) noexcept { m_transform = transform; }
			void SetPos(const Vec3& pos) noexcept { m_transform.pos = pos; }
			void SetRotation(const Vec3& rotation) noexcept { m_transform.rotation = rotation; }

			void SetMatch(std::weak_ptr<GameMatch> match) noexcept { m_match = match; }

			uint32 GetID() const noexcept { return m_id; }
			const std::wstring& GetName() const noexcept { return m_name; }
			const Transform& GetTransform() const noexcept { return m_transform; }
			const Vec3& GetPos() const noexcept { return m_transform.pos; }
			const Vec3& GetRotation() const noexcept { return m_transform.rotation; }
			std::shared_ptr<GameMatch> GetMatch() const noexcept { return m_match.lock(); }

		};
	}

}


