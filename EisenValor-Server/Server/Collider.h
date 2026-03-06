#pragma once

#include "Component.h"

namespace Server {
	namespace Contents {

		class Collider : public Component {
		public:
			explicit Collider(COLLIDER_TYPE type);
			virtual ~Collider();

		public:
			virtual void OnCollisionEnter(Collider* const other);
			virtual void OnCollisionStay(Collider* const  other);
			virtual void OnCollisionExit(Collider* const other);
		
		public:
			uint32 GetID() const { return m_id; }
			COLLIDER_TYPE GetType() const { return m_type; }

		private:
			uint32			m_id;
			COLLIDER_TYPE	m_type;

		};

		class OBBCollider : public Collider {
		public:
			OBBCollider();
			virtual ~OBBCollider();

		public:
			virtual void Update(const float dt) override final;

		public:
			Vec3			GetCenter() const { return m_center; }
			Vec3			GetExtents() const { return m_extents; }
			Quaternion		GetOrientation() const { return m_orientation; }

		private:
			Vec3		m_center;
			Vec3		m_extents;
			Quaternion	m_orientation;

			Vec3		m_localOffset;
			Vec3		m_localExtents;

			Vec3		m_prevPos;
			Vec3		m_prevRot;
			Vec3		m_prevScale;
			bool		m_isDirty;
		};
	}
}


