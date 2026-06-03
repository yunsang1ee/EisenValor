#include "stdafxClient.h"
#include "FootIKComponent.h"

#include "AnimationComponent.h"
#include "AssetLoader.h"
#include "GameObject.h"
#include "GameObject.inl"
#include "MeshData.h"
#include "MeshComponent.h"
#include "MeshResource.h"
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

void FootIKComponent::OnLateUpdate(float)
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

	static std::unordered_set<const FootIKComponent*> loggedComponents;
	if (loggedComponents.insert(this).second)
	{
		auto&		ownerTransform = owner->GetTransform();
		const auto  ownerWorldPosition = ownerTransform.GetWorldPosition();
		const auto  leftFootWorld = TransformBonePositionToWorld(leftFootMatrix.r[3], ownerTransform);
		const auto  leftTargetWorld = TransformBonePositionToWorld(leftTargetMatrix.r[3], ownerTransform);
		const auto  rightFootWorld = TransformBonePositionToWorld(rightFootMatrix.r[3], ownerTransform);
		const auto  rightTargetWorld = TransformBonePositionToWorld(rightTargetMatrix.r[3], ownerTransform);
		DirectX::XMFLOAT3 leftFootWorldPosition;
		DirectX::XMFLOAT3 rightFootWorldPosition;
		DirectX::XMStoreFloat3(&leftFootWorldPosition, leftFootWorld);
		DirectX::XMStoreFloat3(&rightFootWorldPosition, rightFootWorld);
		const DirectX::XMFLOAT3 footCenter = {
			(leftFootWorldPosition.x + rightFootWorldPosition.x) * 0.5f,
			(leftFootWorldPosition.y + rightFootWorldPosition.y) * 0.5f,
			(leftFootWorldPosition.z + rightFootWorldPosition.z) * 0.5f
		};

		/*DEBUG_LOG_FMT(
			"[FootIK] modelY leftFoot={:.3f}, leftIkFoot={:.3f}, rightFoot={:.3f}, rightIkFoot={:.3f}\n",
			DirectX::XMVectorGetY(leftFootMatrix.r[3]), DirectX::XMVectorGetY(leftTargetMatrix.r[3]),
			DirectX::XMVectorGetY(rightFootMatrix.r[3]), DirectX::XMVectorGetY(rightTargetMatrix.r[3])
		);
		DEBUG_LOG_FMT(
			"[FootIK] worldY player={:.3f}, leftFoot={:.3f}, leftIkFoot={:.3f}, rightFoot={:.3f}, rightIkFoot={:.3f}\n",
			ownerWorldPosition.y, DirectX::XMVectorGetY(leftFootWorld), DirectX::XMVectorGetY(leftTargetWorld),
			DirectX::XMVectorGetY(rightFootWorld), DirectX::XMVectorGetY(rightTargetWorld)
		);*/

		if (auto* scene = owner->GetScene())
		{
			if (auto* meshStorage = scene->GetStorage<MeshComponent>())
			{
				//DEBUG_LOG_FMT("[FootIK] scene mesh count={}\n", meshStorage->GetList().size());

				constexpr float nearbyRadius = 5.0f;
				constexpr float nearbyRadiusSq = nearbyRadius * nearbyRadius;
				constexpr float nearbyHeightTolerance = 0.5f;
				size_t nearbyCandidateCount = 0;
				size_t nearbyHeightCandidateCount = 0;
				size_t loggedCandidateCount = 0;
				size_t loggedHeightCandidateCount = 0;
				size_t loggedMeshCount = 0;
				struct HeightCandidate
				{
					const GameObject* obj = nullptr;
					const MeshResource* res = nullptr;
					DirectX::XMFLOAT3 worldPos = {};
					float distanceSqXZ = 0.0f;
					float deltaY = 0.0f;
				};
				std::vector<HeightCandidate> heightCandidates;
				std::unordered_set<EvAsset::Guid, EvAsset::GuidHash> uniqueHeightCandidateGuids;
				const GameObject* closestHeightCandidateObj = nullptr;
				const MeshResource* closestHeightCandidateRes = nullptr;
				DirectX::XMFLOAT3 closestHeightCandidateWorld = {};
				float closestHeightCandidateDistanceSq = std::numeric_limits<float>::max();
				float closestHeightCandidateDeltaY = 0.0f;
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
					if (loggedMeshCount < 12)
					{
						//DEBUG_LOG_FMT(
						//	"[FootIK] scene mesh obj='{}' guid={} world=({:.3f}, {:.3f}, {:.3f})\n",
						//	meshObj->GetName().c_str(), meshRes->GetGuid(), worldPos.x, worldPos.y, worldPos.z
						//);
						++loggedMeshCount;
					}

					const float distanceSqXZ = DistanceSqXZ(worldPos, footCenter);
					if (distanceSqXZ <= nearbyRadiusSq)
					{
						++nearbyCandidateCount;

						if (loggedCandidateCount < 8)
						{
							//DEBUG_LOG_FMT(
							//	"[FootIK] nearby mesh candidate obj='{}' guid={} world=({:.3f}, {:.3f}, {:.3f}) distXZ={:.3f}\n",
							//	meshObj->GetName().c_str(), meshRes->GetGuid(), worldPos.x, worldPos.y, worldPos.z,
							//	std::sqrt(distanceSqXZ)
							//);
							++loggedCandidateCount;
						}

						const float heightDelta = std::fabs(worldPos.y - footCenter.y);
						if (heightDelta <= nearbyHeightTolerance)
						{
							++nearbyHeightCandidateCount;
							uniqueHeightCandidateGuids.insert(meshRes->GetGuid());
							heightCandidates.push_back({meshObj, meshRes, worldPos, distanceSqXZ, heightDelta});

							if (distanceSqXZ < closestHeightCandidateDistanceSq)
							{
								closestHeightCandidateObj = meshObj;
								closestHeightCandidateRes = meshRes;
								closestHeightCandidateWorld = worldPos;
								closestHeightCandidateDistanceSq = distanceSqXZ;
								closestHeightCandidateDeltaY = heightDelta;
							}

							if (loggedHeightCandidateCount < 8)
							{
								DEBUG_LOG_FMT(
									"[FootIK] nearby height candidate obj='{}' guid={} world=({:.3f}, {:.3f}, {:.3f}) distXZ={:.3f} deltaY={:.3f}\n",
									meshObj->GetName().c_str(), meshRes->GetGuid(), worldPos.x, worldPos.y, worldPos.z,
									std::sqrt(distanceSqXZ), heightDelta
								);
								++loggedHeightCandidateCount;
							}
						}
					}

				}

				DEBUG_LOG_FMT(
					"[FootIK] nearby mesh candidate count={} radius={:.1f} footCenter=({:.3f}, {:.3f}, {:.3f})\n",
					nearbyCandidateCount, nearbyRadius, footCenter.x, footCenter.y, footCenter.z
				);
				DEBUG_LOG_FMT(
					"[FootIK] nearby height candidate count={} uniqueGuidCount={} radius={:.1f} heightTolerance={:.1f}\n",
					nearbyHeightCandidateCount, uniqueHeightCandidateGuids.size(), nearbyRadius, nearbyHeightTolerance
				);
				if (closestHeightCandidateObj && closestHeightCandidateRes)
				{
					std::filesystem::path closestHeightCandidatePath;
					DEBUG_LOG_FMT(
						"[FootIK] closest height candidate obj='{}' guid={} world=({:.3f}, {:.3f}, {:.3f}) distXZ={:.3f} deltaY={:.3f}\n",
						closestHeightCandidateObj->GetName().c_str(), closestHeightCandidateRes->GetGuid(),
						closestHeightCandidateWorld.x, closestHeightCandidateWorld.y, closestHeightCandidateWorld.z,
						std::sqrt(closestHeightCandidateDistanceSq), closestHeightCandidateDeltaY
					);
					if (GLOBAL(ResourceGlobal).TryGetPath(closestHeightCandidateRes->GetGuid(), closestHeightCandidatePath))
					{
						DEBUG_LOG_FMT(
							"[FootIK] closest height candidate path={}\n", closestHeightCandidatePath.string()
						);

						EvAsset::MeshData meshData;
						if (EvAsset::AssetLoader::Load(closestHeightCandidatePath, meshData))
						{
							DEBUG_LOG_FMT(
								"[FootIK] closest height mesh loaded vertices={} indices={} subMeshes={}\n",
								meshData.vertices.size(), meshData.indices.size(), meshData.subMeshes.size()
							);
						}
						else
						{
							DEBUG_LOG_FMT("[FootIK] closest height mesh load failed\n");
						}
					}
					else
					{
						DEBUG_LOG_FMT("[FootIK] closest height candidate path lookup failed\n");
					}
				}

				// Ray를 이용한 충돌 검사로 후보군의 실제 높이 샘플링
				const auto rayOrigin = DirectX::XMVectorSet(footCenter.x, footCenter.y + 0.5f, footCenter.z, 1.0f);
				const auto rayDir = DirectX::XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);
				constexpr float rayMaxDistance = 1.5f;
				float bestRayDistance = std::numeric_limits<float>::max();
				DirectX::XMFLOAT3 bestHitPoint = {};
				const HeightCandidate* bestRayCandidate = nullptr;
				size_t rayCheckedCandidateCount = 0;
				size_t rayLoadedCandidateCount = 0;

				for (const auto& candidate : heightCandidates)
				{
					if (!candidate.obj || !candidate.res)
					{
						continue;
					}

					++rayCheckedCandidateCount;
					std::filesystem::path candidatePath;
					if (!GLOBAL(ResourceGlobal).TryGetPath(candidate.res->GetGuid(), candidatePath))
					{
						continue;
					}

					EvAsset::MeshData meshData;
					if (!EvAsset::AssetLoader::Load(candidatePath, meshData))
					{
						continue;
					}
					++rayLoadedCandidateCount;

					const auto worldMatrix = candidate.obj->GetTransform().GetWorldMatrix();
					const auto world = DirectX::XMLoadFloat4x4(&worldMatrix);

					for (size_t index = 0; index + 2 < meshData.indices.size(); index += 3)
					{
						const uint32_t i0 = meshData.indices[index];
						const uint32_t i1 = meshData.indices[index + 1];
						const uint32_t i2 = meshData.indices[index + 2];
						if (i0 >= meshData.vertices.size() || i1 >= meshData.vertices.size() || i2 >= meshData.vertices.size())
						{
							continue;
						}

						const auto v0 = DirectX::XMVector3TransformCoord(
							DirectX::XMVectorSet(
								meshData.vertices[i0].position[0], meshData.vertices[i0].position[1],
								meshData.vertices[i0].position[2], 1.0f
							),
							world
						);
						const auto v1 = DirectX::XMVector3TransformCoord(
							DirectX::XMVectorSet(
								meshData.vertices[i1].position[0], meshData.vertices[i1].position[1],
								meshData.vertices[i1].position[2], 1.0f
							),
							world
						);
						const auto v2 = DirectX::XMVector3TransformCoord(
							DirectX::XMVectorSet(
								meshData.vertices[i2].position[0], meshData.vertices[i2].position[1],
								meshData.vertices[i2].position[2], 1.0f
							),
							world
						);

						float distance = 0.0f;
						if (!DirectX::TriangleTests::Intersects(rayOrigin, rayDir, v0, v1, v2, distance))
						{
							continue;
						}

						if (distance < 0.0f || distance > rayMaxDistance || distance >= bestRayDistance)
						{
							continue;
						}

						bestRayDistance = distance;
						bestRayCandidate = &candidate;
						const auto hitPoint =
							DirectX::XMVectorMultiplyAdd(rayDir, DirectX::XMVectorReplicate(distance), rayOrigin);
						DirectX::XMStoreFloat3(&bestHitPoint, hitPoint);
					}
				}

				DEBUG_LOG_FMT(
					"[FootIK] raycast scan checkedCandidates={} loadedMeshes={} maxDistance={:.2f}\n",
					rayCheckedCandidateCount, rayLoadedCandidateCount, rayMaxDistance
				);
				if (bestRayCandidate)
				{
					DEBUG_LOG_FMT(
						"[FootIK] raycast best hit obj='{}' guid={} hit=({:.3f}, {:.3f}, {:.3f}) distance={:.3f} candidateDistXZ={:.3f} candidateDeltaY={:.3f}\n",
						bestRayCandidate->obj->GetName().c_str(), bestRayCandidate->res->GetGuid(), bestHitPoint.x,
						bestHitPoint.y, bestHitPoint.z, bestRayDistance, std::sqrt(bestRayCandidate->distanceSqXZ),
						bestRayCandidate->deltaY
					);
				}
				else
				{
					DEBUG_LOG_FMT("[FootIK] raycast best hit not found\n");
				}
			}
		}
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
	const bool leftHit = TrySampleVisualGround(leftFootWorldPosition, 0.5f, 1.0f, leftGroundHit);
	const bool rightHit = TrySampleVisualGround(rightFootWorldPosition, 0.5f, 1.0f, rightGroundHit);
	/*DEBUG_LOG_FMT(
		"[FootIK] left sample hit={} foot=({:.3f}, {:.3f}, {:.3f}) ground=({:.3f}, {:.3f}, {:.3f}) distance={:.3f}\n",
		leftHit, leftFootWorldPosition.x, leftFootWorldPosition.y, leftFootWorldPosition.z, leftGroundHit.position.x,
		leftGroundHit.position.y, leftGroundHit.position.z, leftGroundHit.distance
	);
	DEBUG_LOG_FMT(
		"[FootIK] right sample hit={} foot=({:.3f}, {:.3f}, {:.3f}) ground=({:.3f}, {:.3f}, {:.3f}) distance={:.3f}\n",
		rightHit, rightFootWorldPosition.x, rightFootWorldPosition.y, rightFootWorldPosition.z,
		rightGroundHit.position.x, rightGroundHit.position.y, rightGroundHit.position.z, rightGroundHit.distance
	);*/

	// 충돌한 지점과 발 위치의 높이 차이를 계산해서 골반 오프셋을 결정
	float desiredPelvisOffsetY = 0.0f;
	if (leftHit)
	{
		desiredPelvisOffsetY = std::min(desiredPelvisOffsetY, leftGroundHit.position.y - leftFootWorldPosition.y);
	}
	if (rightHit)
	{
		desiredPelvisOffsetY = std::min(desiredPelvisOffsetY, rightGroundHit.position.y - rightFootWorldPosition.y);
	}
	m_pelvisOffsetY = std::clamp(desiredPelvisOffsetY + m_footSoleOffset, -m_maxPelvisDrop, 0.0f);
	animation->SetModelRootOffsetY(m_pelvisOffsetY);
	static uint32_t pelvisLogCounter = 0;
	// m_pelvisOffsetY가 얼마인지 출력
	if ((++pelvisLogCounter % 30) == 0)
	{
		DEBUG_LOG_FMT(
			"[FootIK] pelvis offset desired={:.3f} applied={:.3f} maxDrop={:.3f} leftHit={} rightHit={}\n",
			desiredPelvisOffsetY, m_pelvisOffsetY, m_maxPelvisDrop, leftHit, rightHit
		);
	}

	// 발 크기만큼 발바닥 오프셋 적용
	if (m_footSoleOffset == 0.0f)
	{
		m_footSoleOffset = 0.1f;
		DEBUG_LOG_FMT("[FootIK] sole offset initialized applied={:.3f}\n", m_footSoleOffset);
	}

	// 각 발의 타겟 위치를 계산. 충돌이 감지된 경우에는 충돌 지점의 높이에 발바닥 오프셋을 더한 값을 사용
	auto leftTargetPos = leftTargetMatrix.r[3];
	auto rightTargetPos = rightTargetMatrix.r[3];
	if (leftHit)
	{
		const auto groundModel = TransformWorldPositionToModel(DirectX::XMLoadFloat3(&leftGroundHit.position), ownerTransform);
		const float desiredY = DirectX::XMVectorGetY(groundModel) + m_footSoleOffset;
		leftTargetPos = DirectX::XMVectorSetY(leftTargetPos, desiredY);
	}
	if (rightHit)
	{
		const auto groundModel = TransformWorldPositionToModel(DirectX::XMLoadFloat3(&rightGroundHit.position), ownerTransform);
		const float desiredY = DirectX::XMVectorGetY(groundModel) + m_footSoleOffset;
		rightTargetPos = DirectX::XMVectorSetY(rightTargetPos, desiredY);
	}

	// IK Weight 설정. 충돌이 감지된 발은 1.0f, 감지되지 않은 발은 0.0f으로 설정해서 IK가 적용될지 여부를 결정
	m_leftWeight = leftHit ? 1.0f : 0.0f;
	m_rightWeight = rightHit ? 1.0f : 0.0f;

	// 각 발의 Pole Vector를 계산
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
	UpdateHitMarker(this, scene, bestHitPoint);
	static uint32_t hitLogCounter = 0;
	if (bestHitObj && bestHitRes && (++hitLogCounter % 30) == 0)
	{
		std::filesystem::path meshPath;
		const bool hasPath = GLOBAL(ResourceGlobal).TryGetPath(bestHitRes->GetGuid(), meshPath);
		DEBUG_LOG_FMT(
			"[FootIK] sample hit obj='{}' guid={} path='{}' foot=({:.3f},{:.3f},{:.3f}) ground=({:.3f},{:.3f},{:.3f}) distance={:.3f}\n",
			bestHitObj->GetName().c_str(), bestHitRes->GetGuid(), hasPath ? meshPath.string() : "<unresolved>",
			worldPosition.x, worldPosition.y, worldPosition.z, bestHitPoint.x, bestHitPoint.y, bestHitPoint.z,
			bestRayDistance
		);
		DEBUG_LOG_FMT(
			"[FootIK] hit tri v0=({:.3f},{:.3f},{:.3f}) v1=({:.3f},{:.3f},{:.3f}) v2=({:.3f},{:.3f},{:.3f}) normal=({:.3f},{:.3f},{:.3f})\n",
			bestTriV0.x, bestTriV0.y, bestTriV0.z, bestTriV1.x, bestTriV1.y, bestTriV1.z, bestTriV2.x, bestTriV2.y,
			bestTriV2.z, bestHitNormal.x, bestHitNormal.y, bestHitNormal.z
		);
	}
	return true;
}
