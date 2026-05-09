#pragma once
#include <dxcapi.h>
#include <string>
#include <string_view>
#include <unordered_map>
#include <filesystem>
#include <span>

#include "Singleton.h"

// FIXME: 파일 변경 감지 범위 -> 루트만 검사하기에 #include 파일도 추적하도록 변경 필요

class DxShaderCompilerGlobal : public Singleton<DxShaderCompilerGlobal>
{
private:
	friend class Singleton<DxShaderCompilerGlobal>;

	DxShaderCompilerGlobal() = default;
	~DxShaderCompilerGlobal() override = default;

public:
	void Initialize(std::wstring_view cacheDir = L"Build/ShaderCache/");
	void Release() override;

	// 일반 셰이더 컴파일 (VS/PS/CS)
	ComPtr<IDxcBlob> CompileShaderFromFile(
		std::wstring_view									   shaderName,
		std::wstring_view									   filename,
		std::string_view									   entryPoint,
		std::string_view									   target,
		std::span<const std::pair<std::wstring, std::wstring>> defines = {}
	);

	// RT 셰이더 컴파일 (RayGen/Miss/ClosestHit/AnyHit)
	ComPtr<IDxcBlob> CompileRTShader(
		std::wstring_view									   shaderName,
		std::wstring_view									   filename,
		std::string_view									   entryPoint,
		std::span<const std::pair<std::wstring, std::wstring>> defines = {}
	);
	ComPtr<ID3DBlob> AsD3DBlob(IDxcBlob* dxc);

	// 캐시 관리
	void			   InvalidateShader(std::wstring_view shaderName);
	void			   ClearAllCache();
	[[nodiscard]] bool IsShaderCached(std::wstring_view shaderName) const;

private:
	struct WStringHash
	{
		using is_transparent = void;
		size_t operator()(std::wstring_view s) const noexcept { return std::hash<std::wstring_view>{}(s); }
	};

	struct WStringEqual
	{
		using is_transparent = void;
		bool operator()(std::wstring_view a, std::wstring_view b) const noexcept { return a == b; }
	};

	struct ShaderCacheEntry
	{
		ComPtr<IDxcBlob>				blob;
		std::filesystem::file_time_type sourceTimestamp;
		std::wstring					sourceFile;
	};

	[[nodiscard]] ComPtr<IDxcBlob> CompileInternal(
		std::wstring_view									   shaderName,
		std::wstring_view									   filename,
		std::string_view									   entryPoint,
		std::string_view									   target,
		std::span<const std::pair<std::wstring, std::wstring>> defines,
		bool												   isRaytacing
	);

	[[nodiscard]] std::wstring GetCachePath(std::wstring_view shaderName) const;

	[[nodiscard]] bool LoadFromCache(
		std::wstring_view shaderName, std::wstring_view sourceFile, OUT ComPtr<IDxcBlob>& outBlob
	);

	void SaveToCache(std::wstring_view shaderName, std::wstring_view sourceFile, IDxcBlob* blob);

	[[nodiscard]] bool IsSourceModified(std::wstring_view sourceFile, const std::filesystem::file_time_type& cacheTime)
		const;
	[[nodiscard]] bool IsShaderDirectoryModified(
		const std::filesystem::path& directory, const std::filesystem::file_time_type& cacheTime
	) const;

	void LogCompilationError(IDxcBlobEncoding* errorBlob, std::wstring_view shaderName, std::wstring_view filename);

private:
	ComPtr<IDxcCompiler3>	   m_compiler;
	ComPtr<IDxcUtils>		   m_dxcUtils;
	ComPtr<IDxcIncludeHandler> m_includeHandler;

	std::wstring																  m_cacheDirectory;
	std::unordered_map<std::wstring, ShaderCacheEntry, WStringHash, WStringEqual> m_memoryCache;
};
