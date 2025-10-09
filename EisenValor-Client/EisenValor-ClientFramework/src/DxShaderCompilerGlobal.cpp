#include "stdafxClientFramework.h"
#include "DxShaderCompilerGlobal.h"
#include <fstream>
#include <ranges>
#include <algorithm>

void DxShaderCompilerGlobal::Initialize(std::wstring_view cacheDir)
{
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_compiler)));
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_dxcUtils)));
	ThrowIfFailed(m_dxcUtils->CreateDefaultIncludeHandler(&m_includeHandler));

	m_cacheDirectory = cacheDir;
	std::filesystem::create_directories(m_cacheDirectory);

	DEBUG_LOG_FMT(
		"[DxShaderCompilerGlobal] Initialized. Cache: {}\n",
		std::string(m_cacheDirectory.begin(), m_cacheDirectory.end())
	);
}

void DxShaderCompilerGlobal::Release()
{
	m_memoryCache.clear();
	m_includeHandler.Reset();
	m_dxcUtils.Reset();
	m_compiler.Reset();
}

ComPtr<ID3DBlob> DxShaderCompilerGlobal::CompileShaderFromFile(
	std::wstring_view									 shaderName,
	std::wstring_view									 filename,
	std::string_view									 entryPoint,
	std::string_view									 target,
	std::span<const std::pair<std::string, std::string>> defines
)
{
	auto dxcBlob = CompileInternal(shaderName, filename, entryPoint, target, defines, false);

	// IDxcBlob → ID3DBlob 변환
	ComPtr<ID3DBlob> d3dBlob;
	ThrowIfFailed(D3DCreateBlob(dxcBlob->GetBufferSize(), &d3dBlob));
	std::memcpy(d3dBlob->GetBufferPointer(), dxcBlob->GetBufferPointer(), dxcBlob->GetBufferSize());

	return d3dBlob;
}

ComPtr<IDxcBlob> DxShaderCompilerGlobal::CompileRTShader(
	std::wstring_view									 shaderName,
	std::wstring_view									 filename,
	std::string_view									 entryPoint,
	std::span<const std::pair<std::string, std::string>> defines
)
{
	return CompileInternal(shaderName, filename, entryPoint, "lib_6_6", defines, true);
}

ComPtr<IDxcBlob> DxShaderCompilerGlobal::CompileInternal(
	std::wstring_view									 shaderName,
	std::wstring_view									 filename,
	std::string_view									 entryPoint,
	std::string_view									 target,
	std::span<const std::pair<std::string, std::string>> defines,
	bool												 isRaytracing
)
{
	const std::wstring shaderKey{shaderName};
	const std::wstring sourceFile{filename};

	// 메모리 캐시 확인
	if (auto it = m_memoryCache.find(shaderKey); it != m_memoryCache.end())
	{
		if (!IsSourceModified(sourceFile, it->second.sourceTimestamp))
		{
			DEBUG_LOG_FMT(
				"[DxShaderCompiler] Cache hit (memory): {}\n", std::string(shaderName.begin(), shaderName.end())
			);
			return it->second.blob;
		}

		DEBUG_LOG_FMT(
			"[DxShaderCompiler] Source modified, recompiling: {}\n", std::string(shaderName.begin(), shaderName.end())
		);
	}

	// 디스크 캐시 확인
	ComPtr<IDxcBlob> cachedBlob;
	if (LoadFromCache(shaderName, sourceFile, cachedBlob))
	{
		auto timestamp = std::filesystem::last_write_time(sourceFile);
		m_memoryCache[shaderKey] = {cachedBlob, timestamp, sourceFile};

		DEBUG_LOG_FMT("[DxShaderCompiler] Cache hit (disk): {}\n", std::string(shaderName.begin(), shaderName.end()));
		return cachedBlob;
	}

	// 파일 읽기
	ComPtr<IDxcBlobEncoding> sourceBlob;
	ThrowIfFailed(m_dxcUtils->LoadFile(sourceFile.c_str(), nullptr, &sourceBlob));

	std::vector<std::wstring> argStrings;
	argStrings.reserve(10 + defines.size() * 2);

	// Entry point
	argStrings.emplace_back(L"-E");
	argStrings.emplace_back(entryPoint.begin(), entryPoint.end());

	// Target
	argStrings.emplace_back(L"-T");
	argStrings.emplace_back(target.begin(), target.end());

	// Defines
	for (const auto& [name, value] : defines)
	{
		argStrings.emplace_back(L"-D");
		argStrings.emplace_back(
			std::format(L"{}={}", std::wstring(name.begin(), name.end()), std::wstring(value.begin(), value.end()))
		);
	}

#ifdef _DEBUG
	constexpr std::array debugFlags{L"-Zi", L"-Od", L"-Qembed_debug"};
	argStrings.insert(argStrings.end(), debugFlags.begin(), debugFlags.end());
#else
	argStrings.emplace_back(L"-O3");
#endif

	constexpr std::array commonFlags{L"-WX", L"-enable-16bit-types"};
	argStrings.insert(argStrings.end(), commonFlags.begin(), commonFlags.end());

	std::vector<const wchar_t*> args(argStrings.size());
	std::transform(argStrings.begin(), argStrings.end(), args.begin(), [](const auto& str) { return str.c_str(); });

	const DxcBuffer sourceBuffer{
		.Ptr = sourceBlob->GetBufferPointer(), .Size = sourceBlob->GetBufferSize(), .Encoding = DXC_CP_UTF8
	};

	ComPtr<IDxcResult> result;
	ThrowIfFailed(m_compiler->Compile(
		&sourceBuffer, args.data(), static_cast<UINT32>(args.size()), m_includeHandler.Get(), IID_PPV_ARGS(&result)
	));

	HRESULT hrStatus;
	result->GetStatus(&hrStatus);
	if (FAILED(hrStatus))
	{
		if (ComPtr<IDxcBlobEncoding> errorBlob;
			SUCCEEDED(result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errorBlob), nullptr)))
		{
			LogCompilationError(errorBlob.Get(), shaderName, filename);
		}
		ThrowIfFailed(hrStatus);
	}

	ComPtr<IDxcBlob> shader;
	result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader), nullptr);
	if (!shader)
	{
		DEBUG_LOG_FMT(
			"[DxShaderCompiler][ERROR] No shader output for: {}\n", std::string(shaderName.begin(), shaderName.end())
		);
	}

	SaveToCache(shaderName, sourceFile, shader.Get());
	auto timestamp = std::filesystem::last_write_time(sourceFile);
	m_memoryCache[shaderKey] = {shader, timestamp, sourceFile};

	DEBUG_LOG_FMT(
		"[DxShaderCompiler] Compiled: {} (Entry: {}, Target: {})\n", std::string(shaderName.begin(), shaderName.end()),
		std::string(entryPoint.begin(), entryPoint.end()), std::string(target.begin(), target.end())
	);

	return shader;
}

void DxShaderCompilerGlobal::InvalidateShader(std::wstring_view shaderName)
{
	const std::wstring key{shaderName};
	m_memoryCache.erase(key);

	if (auto cachePath = GetCachePath(shaderName); std::filesystem::exists(cachePath))
	{
		std::filesystem::remove(cachePath);
	}

	DEBUG_LOG_FMT("[DxShaderCompiler] Invalidated: {}\n", std::string(shaderName.begin(), shaderName.end()));
}

void DxShaderCompilerGlobal::ClearAllCache()
{
	m_memoryCache.clear();
	std::filesystem::remove_all(m_cacheDirectory);
	std::filesystem::create_directories(m_cacheDirectory);
	DEBUG_LOG_FMT("[DxShaderCompiler] Cleared all cache.\n");
}

bool DxShaderCompilerGlobal::IsShaderCached(std::wstring_view shaderName) const
{
	return m_memoryCache.contains(std::wstring{shaderName});
}

std::wstring DxShaderCompilerGlobal::GetCachePath(std::wstring_view shaderName) const
{
	std::wstring safeName{shaderName};

	std::replace_if(
		safeName.begin(), safeName.end(), [](wchar_t c) { return c == L'/' || c == L'\\' || c == L':'; }, L'_'
	);

	return std::format(L"{}{}.dxc", m_cacheDirectory, safeName);
}

bool DxShaderCompilerGlobal::LoadFromCache(
	std::wstring_view shaderName, std::wstring_view sourceFile, OUT ComPtr<IDxcBlob>& outBlob
)
{
	const auto cachePath = GetCachePath(shaderName);
	if (!std::filesystem::exists(cachePath))
		return false;

	const auto cacheTime = std::filesystem::last_write_time(cachePath);
	if (IsSourceModified(sourceFile, cacheTime))
		return false;

	std::ifstream file(cachePath, std::ios::binary | std::ios::ate);
	if (!file)
		return false;

	const auto fileSize = static_cast<size_t>(file.tellg());
	file.seekg(0);

	std::vector<uint8_t> buffer(fileSize);
	file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

	ComPtr<IDxcBlobEncoding> loadedBlob;
	ThrowIfFailed(m_dxcUtils->CreateBlob(buffer.data(), static_cast<uint32_t>(fileSize), DXC_CP_ACP, &loadedBlob));

	outBlob = loadedBlob;
	return true;
}

void DxShaderCompilerGlobal::SaveToCache(std::wstring_view shaderName, std::wstring_view sourceFile, IDxcBlob* blob)
{
	const auto	  cachePath = GetCachePath(shaderName);
	std::ofstream file(cachePath, std::ios::binary);

	if (!file)
	{
		DEBUG_LOG_FMT(
			"[DxShaderCompiler][WARN] Failed to save cache: {}\n", std::string(shaderName.begin(), shaderName.end())
		);
		return;
	}

	file.write(static_cast<const char*>(blob->GetBufferPointer()), blob->GetBufferSize());
}

bool DxShaderCompilerGlobal::IsSourceModified(
	std::wstring_view sourceFile, const std::filesystem::file_time_type& cacheTime
) const
{
	const std::wstring path{sourceFile};

	if (!std::filesystem::exists(path))
		return true;

	return std::filesystem::last_write_time(path) > cacheTime;
}

void DxShaderCompilerGlobal::LogCompilationError(
	IDxcBlobEncoding* errorBlob, std::wstring_view shaderName, std::wstring_view filename
)
{
	if (!errorBlob || errorBlob->GetBufferSize() == 0)
		return;

	const std::string_view errorMsg{
		static_cast<const char*>(errorBlob->GetBufferPointer()), errorBlob->GetBufferSize()
	};

	DEBUG_LOG_FMT(
		"[DxShaderCompiler][ERROR] Shader: {}\n  File: {}\n{}\n", std::string(shaderName.begin(), shaderName.end()),
		std::string(filename.begin(), filename.end()), errorMsg
	);
}
