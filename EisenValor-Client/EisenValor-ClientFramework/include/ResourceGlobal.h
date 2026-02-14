#pragma once
#include "Singleton.h"
#include "IResource.h"
#include "Commonutils.h"
#include <unordered_map>
#include <memory>
#include <filesystem>

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
	template <IsValidResource T>
	std::shared_ptr<T> Load(const std::filesystem::path& path)
	{
		std::filesystem::path finalPath = path;
		if (finalPath.is_relative())
		{
			finalPath = Utils::ExeDir() / finalPath;
		}

		std::wstring pathStr = finalPath.wstring();

		if (m_pathToGuid.contains(pathStr))
		{
			return GetResource<T>(m_pathToGuid[pathStr]);
		}

		std::shared_ptr<T> resource = LoadInternal<T>(finalPath);

		if (resource)
		{
			const EvAsset::Guid& guid = resource->GetGuid();
			m_resourceCache[guid] = resource;
			m_pathToGuid[pathStr] = guid;
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

private:
	template <typename T>
	std::shared_ptr<T> LoadInternal(const std::filesystem::path& path);

private:
	struct GuidHash
	{
		size_t operator()(const EvAsset::Guid& guid) const
		{
			return std::hash<uint64_t>{}(guid.low) ^ (std::hash<uint64_t>{}(guid.high) << 1);
		}
	};

	std::unordered_map<EvAsset::Guid, std::shared_ptr<IResource>, GuidHash> m_resourceCache;
	std::unordered_map<std::wstring, EvAsset::Guid>							m_pathToGuid;
};

class MeshResource;
class TextureResource;
class MaterialResource;
class AnimationResource;

template <>
std::shared_ptr<MeshResource> ResourceGlobal::LoadInternal<MeshResource>(const std::filesystem::path& path);
template <>
std::shared_ptr<TextureResource> ResourceGlobal::LoadInternal<TextureResource>(const std::filesystem::path& path);
template <>
std::shared_ptr<MaterialResource> ResourceGlobal::LoadInternal<MaterialResource>(const std::filesystem::path& path);
template <>
std::shared_ptr<AnimationResource> ResourceGlobal::LoadInternal<AnimationResource>(const std::filesystem::path& path);
