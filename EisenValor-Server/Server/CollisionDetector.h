#pragma once
#include "Collider.h"

namespace Server {
	namespace Contents {

		class GameWorld;

		class Collider;
		class OBBCollider;

		class CollisionDetector {
		private:
			using CollisionFunc = bool(*)(const Collider* const, const Collider* const);
			static constexpr uint8 MAX_TYPE_COUNT = static_cast<int>(COLLIDER_TYPE::END);

		private:
			CollisionDetector();
			CollisionDetector(const CollisionDetector&) = delete;
			CollisionDetector(CollisionDetector&&) = delete;
			CollisionDetector& operator=(const CollisionDetector&) = delete;
			CollisionDetector& operator=(CollisionDetector&&) = delete;

		public:
			bool CheckCollision(const Collider* const left, const Collider* const right);

		private:
			void Init();
		
		private:
			static bool Check_OBB_OBB(const Collider* const left, const Collider* const right);

		private:
			static bool OverlapOnAxis(const OBBCollider* const box1, const OBBCollider* const box2, const Matrix& mat1, const Matrix& mat2, const Vec3& axis);
		
			friend class GameWorld;

		private:
			CollisionFunc m_collisionTable[MAX_TYPE_COUNT + 1][MAX_TYPE_COUNT + 1];

		};
	}
}