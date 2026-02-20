#pragma once
#include "Singleton.h"
#include "IResource.h"
#include "Commonutils.h"
#include <unordered_map>
#include <memory>
#include <filesystem>
#include <queue>

class ResourceGlobal : public Singleton<ResourceGlobal>
{
	friend class Singleton<ResourceGlobal>;
	ResourceGlobal() = default;
	~ResourceGlobal() override = default;

public:
	void Initialize() override;
	void Release() override;

	// ---------------------------------------------------------
	// 통합 로더 인터페이스 (Template)
	// ---------------------------------------------------------
	bool LoadRegistry(const std::filesystem::path& path);

	//GUID
	template <IsValidResource T>
	std::shared_ptr<T> Load(const EvAsset::Guid& guid)
	{
		if (auto cache = GetResource<T>(guid))
		{
			return cache;
		}

		auto it = m_guidToPath.find(guid);
		if (m_guidToPath.end() == it)
		{
			return nullptr;
		}

		std::shared_ptr<T> resource = LoadInternal<T>(it->second);
		if (resource)
		{
			m_resourceCache[guid] = resource;
			m_pathToGuid[it->second.wstring()] = guid;
			// 마지막 로딩 시간을 m_guidToLastWriteTime에 저장
			m_guidToLastWriteTime[guid] = std::filesystem::last_write_time(it->second);
		}

		return resource;
	}
	
	//경로
	template <IsValidResource T>
	std::shared_ptr<T> Load(const std::filesystem::path& path)
	{
		std::filesystem::path finalPath = path;
		if (finalPath.is_relative())
		{
			finalPath = Utils::ExeDir() / finalPath;
		}

		const std::wstring pathKey = finalPath.wstring();

		if (m_pathToGuid.contains(pathKey))
		{
			return Load<T>(m_pathToGuid[pathKey]);
		}

		std::shared_ptr<T> resource = LoadInternal<T>(finalPath);
		if (resource)
		{
			const EvAsset::Guid& guid = resource->GetGuid();
			m_resourceCache[guid] = resource;
			m_pathToGuid[pathKey] = guid;
			m_guidToLastWriteTime[guid] = std::filesystem::last_write_time(finalPath);
		}

		return resource;
	}

	template <IsValidResource T>
	std::shared_ptr<T> GetResource(const EvAsset::Guid& guid)
	{
		auto it = m_resourceCache.find(guid);
		if (m_resourceCache.end() != it)
		{
			if (T::StaticRuntimeTypeID() == it->second->GetRuntimeTypeID())
			{
				return std::static_pointer_cast<T>(it->second);
			}
		}
		return nullptr;
	}

	void ProcessPendingLoads();	// 리소스 예약
	void CheckForReload();	//Hot Reload

private:
	template <typename T>
	std::shared_ptr<T> LoadInternal(const std::filesystem::path& path);

	// 로딩 예약 정보
	struct LoadingTask
	{
		std::shared_ptr<IResource> targetResource; // 리소스 껍데기, 실제 GPU 업로드는 ProcessPendingLoads에서 처리,  // shared_ptr로 관리하여 핫 리로드에도 유리
		EvAsset::Guid assetGuid;	// 검증용
		std::filesystem::path path;
		ResourceTypeID typeID;	// 리소스 타입 식별자
	};

private:
	//struct GuidHash
	//{
	//	size_t operator()(const EvAsset::Guid& guid) const
	//	{
	//		return std::hash<uint64_t>{}(guid.low) ^ (std::hash<uint64_t>{}(guid.high) << 1);
	//	}
	//};

	std::unordered_map<EvAsset::Guid, std::shared_ptr<IResource>, EvAsset::GuidHash> m_resourceCache;
	std::unordered_map<EvAsset::Guid, std::filesystem::path, EvAsset::GuidHash>		 m_guidToPath;
	std::unordered_map<EvAsset::Guid, std::filesystem::file_time_type, EvAsset::GuidHash> m_guidToLastWriteTime;
	std::unordered_map<std::wstring, EvAsset::Guid>									 m_pathToGuid;

	std::queue<LoadingTask> m_pendingLoads;
};

class MeshResource;
class TextureResource;
class MaterialResource;
class AnimationResource;
class SkinnedMeshResource;
class SceneResource;

template <>
std::shared_ptr<MeshResource> ResourceGlobal::LoadInternal<MeshResource>(const std::filesystem::path& path);
template <>
std::shared_ptr<TextureResource> ResourceGlobal::LoadInternal<TextureResource>(const std::filesystem::path& path);
template <>
std::shared_ptr<MaterialResource> ResourceGlobal::LoadInternal<MaterialResource>(const std::filesystem::path& path);
template <>
std::shared_ptr<AnimationResource> ResourceGlobal::LoadInternal<AnimationResource>(const std::filesystem::path& path);
template <>
std::shared_ptr<SkinnedMeshResource> ResourceGlobal::LoadInternal<SkinnedMeshResource>(const std::filesystem::path& path);
template <>
std::shared_ptr<SceneResource> ResourceGlobal::LoadInternal<SceneResource>(const std::filesystem::path& path);
