#pragma once
#include <dxcapi.h>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <span>

#include "Singleton.h"

class DxShaderCompilerGlobal : public Singleton<DxShaderCompilerGlobal>
{
public:
	void Initialize(std::wstring_view cacheDir = L"Build/ShaderCache/");
	void Release();

	// 일반 셰이더 컴파일 (VS/PS/CS)
	ComPtr<ID3DBlob> CompileShaderFromFile(
		std::wstring_view									 shaderName,
		std::wstring_view									 filename,
		std::string_view									 entryPoint,
		std::string_view									 target,
		std::span<const std::pair<std::string, std::string>> defines = {}
	);

	// RT 셰이더 컴파일 (RayGen/Miss/ClosestHit/AnyHit)
	ComPtr<IDxcBlob> CompileRTShader(
		std::wstring_view									 shaderName,
		std::wstring_view									 filename,
		std::string_view									 entryPoint,
		std::span<const std::pair<std::string, std::string>> defines = {}
	);


	// 캐시 관리

	void			   InvalidateShader(std::wstring_view shaderName);
	void			   ClearAllCache();
	[[nodiscard]] bool IsShaderCached(std::wstring_view shaderName) const;

private:
	struct ShaderCacheEntry
	{
		ComPtr<IDxcBlob>				blob;
		std::filesystem::file_time_type sourceTimestamp;
		std::wstring					sourceFile;
	};

	[[nodiscard]] ComPtr<IDxcBlob> CompileInternal(
		std::wstring_view									 shaderName,
		std::wstring_view									 filename,
		std::string_view									 entryPoint,
		std::string_view									 target,
		std::span<const std::pair<std::string, std::string>> defines,
		bool												 isRaytracing
	);

	[[nodiscard]] std::wstring GetCachePath(std::wstring_view shaderName) const;

	[[nodiscard]] bool LoadFromCache(
		std::wstring_view shaderName, std::wstring_view sourceFile, OUT ComPtr<IDxcBlob>& outBlob
	);

	void SaveToCache(std::wstring_view shaderName, std::wstring_view sourceFile, IDxcBlob* blob);

	[[nodiscard]] bool IsSourceModified(std::wstring_view sourceFile, const std::filesystem::file_time_type& cacheTime)
		const;

	void LogCompilationError(IDxcBlobEncoding* errorBlob, std::wstring_view shaderName, std::wstring_view filename);

private:
	ComPtr<IDxcCompiler3>	   m_compiler;
	ComPtr<IDxcUtils>		   m_dxcUtils;
	ComPtr<IDxcIncludeHandler> m_includeHandler;
	std::wstring			   m_cacheDirectory;

	std::unordered_map<std::wstring, ShaderCacheEntry> m_memoryCache;
};
