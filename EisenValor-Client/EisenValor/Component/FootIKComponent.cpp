#include "stdafxClient.h"
#include "FootIKComponent.h"

#include "AnimationComponent.h"
#include "AssetLoader.h"
#include "GameObject.h"
#include "GameObject.inl"
#include "MeshData.h"
#include "MeshComponent.h"
#include "MeshResource.h"
#include "MovementComponent.h"
#include "ResourceGlobal.h"
#include "Scene.h"
#include "Transform.h"

#include <DirectXCollision.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace
{
bool TryGetBone(AnimationComponent& animation, const char* boneName, uint32_t& outIndex)
{
	if (animation.GetBoneIndexByName(boneName, outIndex))
	{
		return true;
	}

	DEBUG_LOG_FMT("[FootIK] Missing bone '{}'\n", boneName);
	outIndex = UINT32_MAX;
	return false;
}

// 유효한 뼈 인덱스인지 확인하고 해당 뼈의 Pre-IK 매트릭스를 가져옴 (원래 뼈 행렬)
bool TryGetPreIKBoneMatrix(AnimationComponent& animation, uint32_t boneIndex, DirectX::XMMATRIX& outMatrix)
{
	return boneIndex != UINT32_MAX && animation.GetPreIKSocketMatrix(boneIndex, outMatrix);
}

// 
DirectX::XMVECTOR TransformBonePositionToWorld(DirectX::FXMVECTOR bonePosition, const Transform& ownerTransform)
{
	const auto ownerWorldMatrix = ownerTransform.GetWorldMatrix();
	return DirectX::XMVector3TransformCoord(bonePosition, DirectX::XMLoadFloat4x4(&ownerWorldMatrix));
}

DirectX::XMVECTOR TransformWorldPositionToModel(DirectX::FXMVECTOR worldPosition, const Transform& ownerTransform)
{
	const auto ownerWorldMatrix = ownerTransform.GetWorldMatrix();
	const auto ownerWorld = DirectX::XMLoadFloat4x4(&ownerWorldMatrix);
	const auto ownerWorldInverse = DirectX::XMMatrixInverse(nullptr, ownerWorld);
	return DirectX::XMVector3TransformCoord(worldPosition, ownerWorldInverse);
}

float DistanceSqXZ(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b)
{
	const float dx = a.x - b.x;
	const float dz = a.z - b.z;
	return dx * dx + dz * dz;
}

float SmoothApproach(float current, float target, float deltaTime, float speed)
{
	const float alpha = std::clamp(deltaTime * speed, 0.0f, 1.0f);
	return current + (target - current) * alpha;
}


std::shared_ptr<EvAsset::MeshData> GetCachedMeshData(const EvAsset::Guid& guid, const std::filesystem::path& path)
{
	static std::unordered_map<EvAsset::Guid, std::shared_ptr<EvAsset::MeshData>, EvAsset::GuidHash> meshDataCache;

	if (auto it = meshDataCache.find(guid); it != meshDataCache.end())
	{
		return it->second;
	}

	auto meshData = std::make_shared<EvAsset::MeshData>();
	if (!EvAsset::AssetLoader::Load(path, *meshData))
	{
		return nullptr;
	}

	meshDataCache[guid] = meshData;
	return meshData;
}

void UpdateHitMarker(const FootIKComponent* ownerComponent, Scene* scene, const DirectX::XMFLOAT3& worldPosition)
{
	if (!ownerComponent || !scene)
	{
		return;
	}

	static std::unordered_map<const FootIKComponent*, GameObject::Handle> markerHandles;

	GameObject* markerObject = nullptr;
	auto		it = markerHandles.find(ownerComponent);
	if (it != markerHandles.end())
	{
		markerObject = scene->TryGetGameObject(it->second);
	}

	if (!markerObject)
	{
		auto markerHandle = scene->ReserveGameObject("FootIK_HitMarker", std::nullopt);
		markerHandles[ownerComponent] = markerHandle;
		scene->CreateComponentWithInit<MeshComponent>(
			markerHandle,
			[](MeshComponent* mesh)
			{
				auto meshRes = GLOBAL(ResourceGlobal).Load<MeshResource>("Resource/Models/Range.evmesh");
				if (meshRes)
				{
					mesh->SetMeshResource(meshRes);
				}
			}
		);

		markerObject = scene->TryGetGameObject(markerHandle);
		if (!markerObject)
		{
			return;
		}

		markerObject->GetTransform().SetScale(0.5f);
	}

	const DirectX::XMFLOAT3 markerWorldPosition = {worldPosition.x, worldPosition.y, worldPosition.z};
	markerObject->GetTransform().SetWorldPosition(markerWorldPosition);
}

} // namespace

void FootIKComponent::OnStart()
{
	if (auto* animation = GetGameObject()->GetComponent<AnimationComponent>())
	{
		m_bonesCached = CacheBones(*animation);
		m_ikEnabled = m_bonesCached;
	}
}

void FootIKComponent::OnLateUpdate(float deltaTime)
{
	if (!m_ikEnabled)
	{
		return;
	}

	auto* owner = GetGameObject();
	auto* animation = owner ? owner->GetComponent<AnimationComponent>() : nullptr;
	if (!animation)
	{
		return;
	}

	if (!m_bonesCached)
	{
		m_bonesCached = CacheBones(*animation);
		m_ikEnabled = m_bonesCached;
	}

	if (!m_bonesCached)
	{
		return;
	}

	// 각 뼈의 Pre-IK 매트릭스를 가져옴 (원래 뼈 행렬)
	DirectX::XMMATRIX leftFootMatrix;
	DirectX::XMMATRIX leftThighMatrix;
	DirectX::XMMATRIX leftCalfMatrix;
	DirectX::XMMATRIX leftTargetMatrix;
	DirectX::XMMATRIX rightFootMatrix;
	DirectX::XMMATRIX rightThighMatrix;
	DirectX::XMMATRIX rightCalfMatrix;
	DirectX::XMMATRIX rightTargetMatrix;
	if (!TryGetPreIKBoneMatrix(*animation, m_leftLeg.thigh, leftThighMatrix) ||
		!TryGetPreIKBoneMatrix(*animation, m_leftLeg.calf, leftCalfMatrix) ||
		!TryGetPreIKBoneMatrix(*animation, m_leftLeg.foot, leftFootMatrix) ||
		!TryGetPreIKBoneMatrix(*animation, m_leftLeg.ikFoot, leftTargetMatrix) ||
		!TryGetPreIKBoneMatrix(*animation, m_rightLeg.thigh, rightThighMatrix) ||
		!TryGetPreIKBoneMatrix(*animation, m_rightLeg.calf, rightCalfMatrix) ||
		!TryGetPreIKBoneMatrix(*animation, m_rightLeg.foot, rightFootMatrix) ||
		!TryGetPreIKBoneMatrix(*animation, m_rightLeg.ikFoot, rightTargetMatrix))
	{
		return;
	}


	// 발 위치를 월드 좌표로 변환
	auto&	   ownerTransform = owner->GetTransform();
	const auto leftFootWorld = TransformBonePositionToWorld(leftFootMatrix.r[3], ownerTransform);
	const auto rightFootWorld = TransformBonePositionToWorld(rightFootMatrix.r[3], ownerTransform);
	DirectX::XMFLOAT3 leftFootWorldPosition;
	DirectX::XMFLOAT3 rightFootWorldPosition;
	DirectX::XMStoreFloat3(&leftFootWorldPosition, leftFootWorld);
	DirectX::XMStoreFloat3(&rightFootWorldPosition, rightFootWorld);

	// 발 위치에서 아래로 레이를 쏴서 지면과의 충돌을 검사하고, 충돌 지점과 거리를 계산
	GroundHit leftGroundHit;
	GroundHit rightGroundHit;
	constexpr float kFootRayMaxUp = -0.05f;
	constexpr float kFootRayMaxDown = 1.0f;
	const bool leftHitValid = TrySampleVisualGround(leftFootWorldPosition, kFootRayMaxUp, kFootRayMaxDown, leftGroundHit);
	const bool rightHitValid = TrySampleVisualGround(rightFootWorldPosition, kFootRayMaxUp, kFootRayMaxDown, rightGroundHit);
	/*DEBUG_LOG_FMT(
		"[FootIK] left sample hit={} foot=({:.3f}, {:.3f}, {:.3f}) ground=({:.3f}, {:.3f}, {:.3f}) distance={:.3f}\n",
		leftHitValid, leftFootWorldPosition.x, leftFootWorldPosition.y, leftFootWorldPosition.z, leftGroundHit.position.x,
		leftGroundHit.position.y, leftGroundHit.position.z, leftGroundHit.distance
	);
	DEBUG_LOG_FMT(
		"[FootIK] right sample hit={} foot=({:.3f}, {:.3f}, {:.3f}) ground=({:.3f}, {:.3f}, {:.3f}) distance={:.3f}\n",
		rightHitValid, rightFootWorldPosition.x, rightFootWorldPosition.y, rightFootWorldPosition.z,
		rightGroundHit.position.x, rightGroundHit.position.y, rightGroundHit.position.z, rightGroundHit.distance
	);*/

	// 발 크기만큼 발바닥 오프셋 적용
	if (m_footSoleOffset == 0.0f)
	{
		m_footSoleOffset = 0.1f;
		// DEBUG_LOG_FMT("[FootIK] sole offset initialized applied={:.3f}\n", m_footSoleOffset);
	}

	// 골반
	// 양발 targetGap이 모두 크면 공중, 비슷하면 양발 지지, 차이나면 더 가까운 발만 지지
	constexpr float kPelvisAirborneTargetGap = 0.6f;
	constexpr float kPelvisBothFeetGapTolerance = 0.01f;
	constexpr float kStationaryHorizontalSpeed = 1.0f;
	const float leftTargetWorldY = leftGroundHit.position.y + m_footSoleOffset;
	const float rightTargetWorldY = rightGroundHit.position.y + m_footSoleOffset;
	const float leftTargetGap =
		leftHitValid ? leftFootWorldPosition.y - leftTargetWorldY : std::numeric_limits<float>::max();
	const float rightTargetGap =
		rightHitValid ? rightFootWorldPosition.y - rightTargetWorldY : std::numeric_limits<float>::max();
	const float closestTargetGap = std::min(leftTargetGap, rightTargetGap);
	const float leftTargetGapDelta = m_hasPreviousTargetGap ? leftTargetGap - m_prevLeftTargetGap : 0.0f;
	const float rightTargetGapDelta = m_hasPreviousTargetGap ? rightTargetGap - m_prevRightTargetGap : 0.0f;
	const bool isPelvisAirborne =
		leftTargetGap > kPelvisAirborneTargetGap && rightTargetGap > kPelvisAirborneTargetGap;
	const bool hasSimilarTargetGaps =
		std::abs(leftTargetGap - rightTargetGap) <= kPelvisBothFeetGapTolerance;
	const auto* movement = owner ? owner->GetComponent<MovementComponent>() : nullptr;
	const DirectX::XMFLOAT3 velocity = movement ? movement->GetVelocity() : DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f};
	const float horizontalSpeedSq = velocity.x * velocity.x + velocity.z * velocity.z;
	const bool isStationary = horizontalSpeedSq <= kStationaryHorizontalSpeed * kStationaryHorizontalSpeed;
	const bool leftPelvisSupport =
		leftHitValid && (isStationary || (!isPelvisAirborne && (hasSimilarTargetGaps || leftTargetGap == closestTargetGap)));
	const bool rightPelvisSupport =
		rightHitValid && (isStationary || (!isPelvisAirborne && (hasSimilarTargetGaps || rightTargetGap == closestTargetGap)));

	float desiredPelvisOffsetY = 0.0f;
	bool hasPelvisSupport = false;
	if (leftPelvisSupport && (!rightPelvisSupport || leftTargetWorldY <= rightTargetWorldY))
	{
		desiredPelvisOffsetY = leftTargetWorldY - leftFootWorldPosition.y;
		hasPelvisSupport = true;
	}
	else if (rightPelvisSupport)
	{
		desiredPelvisOffsetY = rightTargetWorldY - rightFootWorldPosition.y;
		hasPelvisSupport = true;
	}
	if (hasPelvisSupport)
	{
		desiredPelvisOffsetY = std::clamp(desiredPelvisOffsetY, -m_maxPelvisDrop, 0.0f);
	}
	m_pelvisOffsetY = SmoothApproach(m_pelvisOffsetY, desiredPelvisOffsetY, deltaTime, 5.0f);
	animation->SetModelRootOffsetY(m_pelvisOffsetY);
	static uint32_t pelvisLogCounter = 0;
	// m_pelvisOffsetY가 얼마인지 출력
	if ((++pelvisLogCounter % 30) == 0)
	{
		DEBUG_LOG_FMT(
			"[FootIK] pelvis offset desired={:.3f} applied={:.3f} maxDrop={:.3f} stationary={} speedSq={:.4f} leftHit={} rightHit={} leftSupport={} rightSupport={} leftGap={:.3f} rightGap={:.3f} leftGapDelta={:.3f} rightGapDelta={:.3f}\n",
			desiredPelvisOffsetY, m_pelvisOffsetY, m_maxPelvisDrop, isStationary, horizontalSpeedSq, leftHitValid, rightHitValid, leftPelvisSupport,
			rightPelvisSupport, leftTargetGap, rightTargetGap, leftTargetGapDelta, rightTargetGapDelta
		);
	}

	m_prevLeftTargetGap = leftTargetGap;
	m_prevRightTargetGap = rightTargetGap;
	m_hasPreviousTargetGap = true;

	// 양발
	auto leftTargetPos = leftFootMatrix.r[3];
	auto rightTargetPos = rightFootMatrix.r[3];
	if (leftHitValid)
	{
		const auto groundModel = TransformWorldPositionToModel(DirectX::XMLoadFloat3(&leftGroundHit.position), ownerTransform);
		const float targetModelY = DirectX::XMVectorGetY(groundModel) + m_footSoleOffset;
		leftTargetPos = DirectX::XMVectorSetY(leftTargetPos, targetModelY);
	}
	if (rightHitValid)
	{
		const auto groundModel = TransformWorldPositionToModel(DirectX::XMLoadFloat3(&rightGroundHit.position), ownerTransform);
		const float targetModelY = DirectX::XMVectorGetY(groundModel) + m_footSoleOffset;
		rightTargetPos = DirectX::XMVectorSetY(rightTargetPos, targetModelY);
	}

	// IK Weight 설정
	// 지지하는 발이 있으면 그 발에 IK를 적용, 없으면 IK 비적용
	m_leftWeight = SmoothApproach(m_leftWeight, leftPelvisSupport ? 1.0f : 0.0f, deltaTime, 5.0f);
	m_rightWeight = SmoothApproach(m_rightWeight, rightPelvisSupport ? 1.0f : 0.0f, deltaTime, 5.0f);

	// 원본 애니메이션의 무릎 방향을 힌트로 넘기기
	const auto leftPoleVector = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(leftCalfMatrix.r[3], leftThighMatrix.r[3]));
	const auto rightPoleVector =
		DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(rightCalfMatrix.r[3], rightThighMatrix.r[3]));

	auto buildLegIKTarget = [](const LegBoneCache& cache, DirectX::FXMVECTOR targetPos, DirectX::FXMVECTOR poleVector, float weight)
	{
		IKTarget target;
		target.rootBoneIndex = cache.thigh;
		target.midBoneIndex = cache.calf;
		target.boneIndex = cache.foot;
		target.targetPos = targetPos;
		target.poleVector = poleVector;
		target.weight = weight;
		target.active = true;
		return target;
	};

	animation->SetIKTarget(
		IK_TYPE::LEFT_LEG, buildLegIKTarget(m_leftLeg, leftTargetPos, leftPoleVector, m_leftWeight)
	);
	animation->SetIKTarget(
		IK_TYPE::RIGHT_LEG, buildLegIKTarget(m_rightLeg, rightTargetPos, rightPoleVector, m_rightWeight)
	);
}

bool FootIKComponent::CacheBones(AnimationComponent& animation)
{
	bool success = true;

	success &= TryGetBone(animation, "thigh_l", m_leftLeg.thigh);
	success &= TryGetBone(animation, "calf_l", m_leftLeg.calf);
	success &= TryGetBone(animation, "foot_l", m_leftLeg.foot);
	success &= TryGetBone(animation, "ik_foot_l", m_leftLeg.ikFoot);

	success &= TryGetBone(animation, "thigh_r", m_rightLeg.thigh);
	success &= TryGetBone(animation, "calf_r", m_rightLeg.calf);
	success &= TryGetBone(animation, "foot_r", m_rightLeg.foot);
	success &= TryGetBone(animation, "ik_foot_r", m_rightLeg.ikFoot);

	success &= TryGetBone(animation, "ik_foot_root", m_ikFootRoot);

	if (!success)
	{
		DEBUG_LOG_FMT("[FootIK] Bone cache failed. IK targets will stay disabled.\n");
	}

	return success && IsValidLegCache(m_leftLeg) && IsValidLegCache(m_rightLeg);
}

bool FootIKComponent::IsValidLegCache(const LegBoneCache& cache) const
{
	return cache.thigh != kInvalidBoneIndex && cache.calf != kInvalidBoneIndex && cache.foot != kInvalidBoneIndex &&
		   cache.ikFoot != kInvalidBoneIndex;
}

bool FootIKComponent::TrySampleVisualGround(
	const DirectX::XMFLOAT3& worldPosition, float maxUp, float maxDown, GroundHit& outHit
) const
{
	auto* owner = GetGameObject();
	auto* scene = owner ? owner->GetScene() : nullptr;
	auto* meshStorage = scene ? scene->GetStorage<MeshComponent>() : nullptr;
	if (!meshStorage)
	{
		return false;
	}

	constexpr float nearbyRadius = 15.0f;
	constexpr float nearbyRadiusSq = nearbyRadius * nearbyRadius;
	const auto	  rayOrigin = DirectX::XMVectorSet(worldPosition.x, worldPosition.y + maxUp, worldPosition.z, 1.0f);
	const auto	  rayDir = DirectX::XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);
	const float	  rayMaxDistance = maxUp + maxDown;
	float		  bestRayDistance = std::numeric_limits<float>::max();
	DirectX::XMFLOAT3 bestHitPoint = {};
	DirectX::XMFLOAT3 bestHitNormal = {0.0f, 1.0f, 0.0f};
	DirectX::XMFLOAT3 bestTriV0 = {};
	DirectX::XMFLOAT3 bestTriV1 = {};
	DirectX::XMFLOAT3 bestTriV2 = {};
	const GameObject* bestHitObj = nullptr;
	const MeshResource* bestHitRes = nullptr;
	size_t		  nearbyMeshCount = 0;
	size_t		  pathResolvedCount = 0;
	size_t		  loadedMeshCount = 0;
	size_t		  triangleTestCount = 0;
	size_t		  triangleIntersectCount = 0;

	for (const auto& meshComp : meshStorage->GetList())
	{
		if (!meshComp.IsValid())
		{
			continue;
		}

		auto* meshObj = meshComp.GetGameObject();
		auto* meshRes = meshComp.GetMeshResource();
		if (!meshObj || !meshRes)
		{
			continue;
		}

		const auto worldPos = meshObj->GetTransform().GetWorldPosition();
		if (DistanceSqXZ(worldPos, worldPosition) > nearbyRadiusSq)
		{
			continue;
		}
		// Mesh object origins are not reliable surface heights, so keep only the XZ proximity filter here.
		++nearbyMeshCount;

		std::filesystem::path meshPath;
		if (!GLOBAL(ResourceGlobal).TryGetPath(meshRes->GetGuid(), meshPath))
		{
			continue;
		}
		++pathResolvedCount;

		auto meshData = GetCachedMeshData(meshRes->GetGuid(), meshPath);
		if (!meshData)
		{
			continue;
		}
		++loadedMeshCount;

		const auto worldMatrix = meshObj->GetTransform().GetWorldMatrix();
		const auto world = DirectX::XMLoadFloat4x4(&worldMatrix);

		for (size_t index = 0; index + 2 < meshData->indices.size(); index += 3)
		{
			const uint32_t i0 = meshData->indices[index];
			const uint32_t i1 = meshData->indices[index + 1];
			const uint32_t i2 = meshData->indices[index + 2];
			if (i0 >= meshData->vertices.size() || i1 >= meshData->vertices.size() || i2 >= meshData->vertices.size())
			{
				continue;
			}

			const auto v0 = DirectX::XMVector3TransformCoord(
				DirectX::XMVectorSet(
					meshData->vertices[i0].position[0], meshData->vertices[i0].position[1],
					meshData->vertices[i0].position[2], 1.0f
				),
				world
			);
			const auto v1 = DirectX::XMVector3TransformCoord(
				DirectX::XMVectorSet(
					meshData->vertices[i1].position[0], meshData->vertices[i1].position[1],
					meshData->vertices[i1].position[2], 1.0f
				),
				world
			);
			const auto v2 = DirectX::XMVector3TransformCoord(
				DirectX::XMVectorSet(
					meshData->vertices[i2].position[0], meshData->vertices[i2].position[1],
					meshData->vertices[i2].position[2], 1.0f
				),
				world
			);

			float distance = 0.0f;
			++triangleTestCount;
			if (!DirectX::TriangleTests::Intersects(rayOrigin, rayDir, v0, v1, v2, distance))
			{
				continue;
			}
			++triangleIntersectCount;

			if (distance < 0.0f || distance > rayMaxDistance || distance >= bestRayDistance)
			{
				continue;
			}

			const auto hitPoint =
				DirectX::XMVectorMultiplyAdd(rayDir, DirectX::XMVectorReplicate(distance), rayOrigin);
			const float hitY = DirectX::XMVectorGetY(hitPoint);
			constexpr float maxGroundAboveFoot = 0.12f;
			if (hitY > worldPosition.y + maxGroundAboveFoot)
			{
				continue;
			}

			bestRayDistance = distance;
			bestHitObj = meshObj;
			bestHitRes = meshRes;
			DirectX::XMStoreFloat3(&bestHitPoint, hitPoint);
			DirectX::XMStoreFloat3(&bestTriV0, v0);
			DirectX::XMStoreFloat3(&bestTriV1, v1);
			DirectX::XMStoreFloat3(&bestTriV2, v2);
			const auto edge01 = DirectX::XMVectorSubtract(v1, v0);
			const auto edge02 = DirectX::XMVectorSubtract(v2, v0);
			const auto normal = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(edge01, edge02));
			DirectX::XMStoreFloat3(&bestHitNormal, normal);
		}
	}

	if (bestRayDistance == std::numeric_limits<float>::max())
	{
		static uint32_t missLogCounter = 0;
		if ((++missLogCounter % 30) == 0)
		{
			//DEBUG_LOG_FMT(
			//	"[FootIK] sample miss foot=({:.3f},{:.3f},{:.3f}) up={:.2f} down={:.2f} nearby={} path={} loaded={} triTest={} triHit={}\n",
			//	worldPosition.x, worldPosition.y, worldPosition.z, maxUp, maxDown, nearbyMeshCount, pathResolvedCount,
			//	loadedMeshCount, triangleTestCount, triangleIntersectCount
			//);
		}
		return false;
	}

	outHit.position = bestHitPoint;
	outHit.normal = bestHitNormal;
	outHit.distance = bestRayDistance;
	//UpdateHitMarker(this, scene, bestHitPoint);
	static uint32_t hitLogCounter = 0;
	//if (bestHitObj && bestHitRes && (++hitLogCounter % 30) == 0)
	//{
	//	std::filesystem::path meshPath;
	//	const bool hasPath = GLOBAL(ResourceGlobal).TryGetPath(bestHitRes->GetGuid(), meshPath);
	//	DEBUG_LOG_FMT(
	//		"[FootIK] sample hit obj='{}' guid={} path='{}' foot=({:.3f},{:.3f},{:.3f}) ground=({:.3f},{:.3f},{:.3f}) distance={:.3f}\n",
	//		bestHitObj->GetName().c_str(), bestHitRes->GetGuid(), hasPath ? meshPath.string() : "<unresolved>",
	//		worldPosition.x, worldPosition.y, worldPosition.z, bestHitPoint.x, bestHitPoint.y, bestHitPoint.z,
	//		bestRayDistance
	//	);
	//	DEBUG_LOG_FMT(
	//		"[FootIK] hit tri v0=({:.3f},{:.3f},{:.3f}) v1=({:.3f},{:.3f},{:.3f}) v2=({:.3f},{:.3f},{:.3f}) normal=({:.3f},{:.3f},{:.3f})\n",
	//		bestTriV0.x, bestTriV0.y, bestTriV0.z, bestTriV1.x, bestTriV1.y, bestTriV1.z, bestTriV2.x, bestTriV2.y,
	//		bestTriV2.z, bestHitNormal.x, bestHitNormal.y, bestHitNormal.z
	//	);
	//}
	return true;
}
