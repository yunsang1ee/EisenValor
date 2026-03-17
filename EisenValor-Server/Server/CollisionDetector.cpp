#include "pch.h"
#include "CollisionDetector.h"

#include "Collider.h"

Server::Contents::CollisionDetector::CollisionDetector()
{
	Init();
}

bool Server::Contents::CollisionDetector::CheckCollision(const Collider* const left, const Collider* const right)
{
	const uint8 leftType{ static_cast<uint8>(left->GetType()) };
	const uint8 rightType{ static_cast<uint8>(right->GetType()) };
	
	if(leftType == 0 || rightType == 0) return false;

	return m_collisionTable[leftType][rightType](left, right);
}

void Server::Contents::CollisionDetector::Init()
{
	for(uint8 i = 0; i < MAX_TYPE_COUNT; ++i) {
		for(uint8 j = 0; j < MAX_TYPE_COUNT; ++j) {
			m_collisionTable[i][j] = [](const Collider* const, const Collider* const) { return false; };
		}
	}

	m_collisionTable[etou8(COLLIDER_TYPE::OBB)][etou8(COLLIDER_TYPE::OBB)] = Check_OBB_OBB;
}

bool Server::Contents::CollisionDetector::Check_OBB_OBB(const Collider* const left, const Collider* const right)
{
	const OBBCollider* const leftCol{ static_cast<const OBBCollider*>(left) };
	const OBBCollider* const rightCol{ static_cast<const OBBCollider*>(right) };

	// [최적화 1단계] Broad Phase: 경계 구(Sphere) 검사
	const float r1{ leftCol->GetExtents().Length() }; // 대각선 길이의 절반 = 반지름
	const float r2{ rightCol->GetExtents().Length() };

	// 두 점 사이의 거리 제곱 (Vector3 static 함수 사용)
	const float distSq{ Vec3::DistanceSquared(leftCol->GetCenter(), rightCol->GetCenter()) };

	// 두 구의 반지름 합보다 중심 거리가 멀면, 절대 충돌 안 함
	if(distSq > (r1 + r2) * (r1 + r2))  return false;

	// [최적화 2단계] Narrow Phase: SAT (분리 축 이론)
	// 쿼터니언 -> 회전 행렬 변환
	const Matrix mat1{ Matrix::CreateFromQuaternion(leftCol->GetOrientation()) };
	const Matrix mat2{Matrix::CreateFromQuaternion(rightCol->GetOrientation())};

	// 검사할 축 목록 (총 15개)
	// SimpleMath는 벡터 배열로 관리하는 것이 반복문 돌리기 편합니다.
	Vec3 axes[15];

	// Group A: Box1의 로컬 축 3개 (Right, Up, Backward)
	axes[0] = mat1.Right();
	axes[1] = mat1.Up();
	axes[2] = mat1.Backward();

	// Group B: Box2의 로컬 축 3개
	axes[3] = mat2.Right();
	axes[4] = mat2.Up();
	axes[5] = mat2.Backward();

	// Group C: 두 박스 축들의 외적(Cross Product) 축 9개
	int k = 6;
	for(int i = 0; i < 3; ++i) {
		for(int j = 3; j < 6; ++j) {
			// .Cross() 멤버 함수 사용
			axes[k++] = axes[i].Cross(axes[j]);
		}
	}

	// 모든 축에 대해 검사
	for(int i = 0; i < 15; ++i) {
		if(!OverlapOnAxis(leftCol, rightCol, mat1, mat2, axes[i])) {
			return false; // 하나라도 겹치지 않는 축(분리 축)이 있으면 충돌 아님
		}
	}

	// 모든 축에서 겹치면 충돌!
	return true;
}

bool Server::Contents::CollisionDetector::OverlapOnAxis(const OBBCollider* const box1, const OBBCollider* const box2, const Matrix& mat1, const Matrix& mat2, const Vec3& axis)
{
	// 축의 길이가 너무 작으면(0에 가까우면) 검사할 필요 없음 (Pass)
// (평행한 축끼리 외적했을 때 발생)
	constexpr float min{ std::numeric_limits<float>::min() };
	
	if(axis.LengthSquared() < min) return true;

	// 1. 두 OBB 중심 간의 거리 벡터를 해당 축에 투영
	//    SimpleMath는 .Dot() 멤버 함수를 사용합니다.
	// 검사 축 위에서 두 박스 중심 사이의 거리를 구함
	const float projection{ abs((box2->GetCenter() - box1->GetCenter()).Dot(axis)) };

	// 2. OBB1의 반지름(투영 길이) 계산
	//    R1 = e1*|u1·L| + e2*|u2·L| + e3*|u3·L|
	//    mat.Right(), mat.Up(), mat.Backward()로 각 축 벡터를 가져옵니다.


	// 로컬 축들을 검사 축에 내적하여 OBB가 회전된 상태에서 검사 축에 드리우는 길이(반지름)을 구한다

	const Vec3 box1Extents{ box1->GetExtents() };
	const Vec3 box2Extents{ box2->GetExtents() };

	const float r1{ box1Extents.x * abs(mat1.Right().Dot(axis)) +
		box1Extents.y * abs(mat1.Up().Dot(axis)) +
		box1Extents.z * abs(mat1.Backward().Dot(axis)) };	// 검사 축 상에서 박스 1이 차지하는 구간의 절반 길이

	// 3. OBB2의 반지름(투영 길이) 계산
	const float r2{ box2Extents.x * abs(mat2.Right().Dot(axis)) +
		box2Extents.y * abs(mat2.Up().Dot(axis)) +
		box2Extents.z * abs(mat2.Backward().Dot(axis)) }; 	// 검사 축 상에서 박스 2가 차지하는 구간의 절반 길이

	// 4. 중심 거리보다 두 반지름의 합이 작으면 '분리된 것'이다.
	return projection <= (r1 + r2);
}
