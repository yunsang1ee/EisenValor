#include "stdafxClientFramework.h"
#include "DxShaderCompilerGlobal.h"
#include "Commonutils.h"
#include <fstream>
#include <ranges>
#include <algorithm>

void DxShaderCompilerGlobal::Initialize(std::wstring_view cacheDir)
{
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_compiler)));
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_dxcUtils)));
	ThrowIfFailed(m_dxcUtils->CreateDefaultIncludeHandler(&m_includeHandler));

	std::filesystem::path cd = std::filesystem::path(cacheDir);
	if (cd.is_relative())
	{
		cd = Utils::ExeDir() / cd;
	}

	m_cacheDirectory = cd.wstring();
	std::filesystem::create_directories(m_cacheDirectory);

	DEBUG_LOG_FMT(
		"[DxShaderCompilerGlobal] Initialized. Cache: {}\n", std::filesystem::path(m_cacheDirectory).string()
	);
}

void DxShaderCompilerGlobal::Release()
{
	m_memoryCache.clear();
	m_includeHandler.Reset();
	m_dxcUtils.Reset();
	m_compiler.Reset();
}

ComPtr<IDxcBlob> DxShaderCompilerGlobal::CompileShaderFromFile(
	std::wstring_view									   shaderName,
	std::wstring_view									   filename,
	std::string_view									   entryPoint,
	std::string_view									   target,
	std::span<const std::pair<std::wstring, std::wstring>> defines
)
{
	return CompileInternal(shaderName, filename, entryPoint, target, defines, false);
}

ComPtr<IDxcBlob> DxShaderCompilerGlobal::CompileRTShader(
	std::wstring_view									   shaderName,
	std::wstring_view									   filename,
	std::string_view									   entryPoint,
	std::span<const std::pair<std::wstring, std::wstring>> defines
)
{
	return CompileInternal(shaderName, filename, entryPoint, "lib_6_6", defines, true);
}

ComPtr<ID3DBlob> DxShaderCompilerGlobal::AsD3DBlob(IDxcBlob* dxc)
{
	ComPtr<ID3DBlob> d3d;
	ThrowIfFailed(D3DCreateBlob(dxc->GetBufferSize(), &d3d));
	std::memcpy(d3d->GetBufferPointer(), dxc->GetBufferPointer(), dxc->GetBufferSize());
	return d3d;
}

ComPtr<IDxcBlob> DxShaderCompilerGlobal::CompileInternal(
	std::wstring_view									   shaderName,
	std::wstring_view									   filename,
	std::string_view									   entryPoint,
	std::string_view									   target,
	std::span<const std::pair<std::wstring, std::wstring>> defines,
	bool												   isRaytacing
)
{
	std::filesystem::path sourcePath = std::filesystem::path(filename);
	if (sourcePath.is_relative())
	{
		sourcePath = Utils::ExeDir() / sourcePath;
	}
	std::wstring sourceFile = sourcePath.wstring();

	// 메모리 캐시 확인
	if (auto it = m_memoryCache.find(shaderName); it != m_memoryCache.end())
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
		m_memoryCache.try_emplace(std::wstring{shaderName}, cachedBlob, timestamp, sourceFile);

		DEBUG_LOG_FMT("[DxShaderCompiler] Cache hit (disk): {}\n", std::string(shaderName.begin(), shaderName.end()));
		return cachedBlob;
	}

	// 파일 읽기
	ComPtr<IDxcBlobEncoding> sourceBlob;
	UINT32					 codePage = DXC_CP_UTF8;
	
	DEBUG_LOG_FMT("[DxShaderCompiler] Loading file: {}\n", sourcePath.string());
	
	ThrowIfFailed(m_dxcUtils->LoadFile(sourceFile.c_str(), &codePage, &sourceBlob));

	ComPtr<IDxcBlobUtf8> sourceUtf8;
	ThrowIfFailed(m_dxcUtils->GetBlobAsUtf8(sourceBlob.Get(), &sourceUtf8));

	std::vector<std::wstring> stringStorage;
	stringStorage.reserve(32); 

	if (!isRaytacing)
	{
		stringStorage.emplace_back(L"-E");
		stringStorage.emplace_back(entryPoint.begin(), entryPoint.end());
	}

	stringStorage.emplace_back(L"-T");
	stringStorage.emplace_back(target.begin(), target.end());

	for (const auto& [name, value] : defines)
	{
		stringStorage.emplace_back(L"-D");
		std::wstring defineStr;
		defineStr.append(name).append(L"=").append(value);
		stringStorage.push_back(std::move(defineStr));
	}

#ifdef _DEBUG
	stringStorage.emplace_back(L"-Zi");
	stringStorage.emplace_back(L"-Od");
	stringStorage.emplace_back(L"-Qembed_debug");
#else
	stringStorage.emplace_back(L"-O3");
	stringStorage.emplace_back(L"-Qstrip_debug");
#endif
	stringStorage.emplace_back(L"-HV");
	stringStorage.emplace_back(L"2021");
	stringStorage.emplace_back(L"-WX");
	stringStorage.emplace_back(L"-enable-16bit-types");

	stringStorage.emplace_back(L"-I");
	std::filesystem::path includePath = Utils::ExeDir() / L"Resource/Shader";
	stringStorage.emplace_back(includePath.wstring());

	std::vector<const wchar_t*> args;
	args.reserve(stringStorage.size());
	for (const auto& str : stringStorage)
	{
		args.push_back(str.c_str());
	}

	const DxcBuffer sourceBuffer{
		.Ptr = sourceUtf8->GetBufferPointer(), .Size = sourceUtf8->GetBufferSize(), .Encoding = DXC_CP_UTF8
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
		else
		{
			DEBUG_LOG_FMT(
				"[DxShaderCompiler][ERROR] Shader: {} failed with HRESULT: 0x{:08X}, but no error blob available.\n",
				std::string(shaderName.begin(), shaderName.end()), static_cast<uint32_t>(hrStatus)
			);
		}
		ComPtr<IDxcBlobUtf16> outputText;
		if (SUCCEEDED(result->GetOutput(DXC_OUT_TEXT, IID_PPV_ARGS(&outputText), nullptr)) && outputText)
		{
			DEBUG_LOG_FMT(
				"[DxShaderCompiler][INFO] Additional output:\n{}\n",
				std::string(static_cast<const char*>(outputText->GetBufferPointer()), outputText->GetBufferSize())
			);
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
	m_memoryCache.try_emplace(std::wstring{shaderName}, shader, timestamp, sourceFile);

	DEBUG_LOG_FMT(
		"[DxShaderCompiler] Compiled: {} (Entry: {}, Target: {})\n", std::string(shaderName.begin(), shaderName.end()),
		std::string(entryPoint.begin(), entryPoint.end()), std::string(target.begin(), target.end())
	);

	return shader;
}

void DxShaderCompilerGlobal::InvalidateShader(std::wstring_view shaderName)
{
	m_memoryCache.erase(shaderName);

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
	return m_memoryCache.contains(shaderName);
}

std::wstring DxShaderCompilerGlobal::GetCachePath(std::wstring_view shaderName) const
{
	std::wstring safeName{shaderName};

	std::replace_if(
		safeName.begin(), safeName.end(), [](wchar_t c) { return c == L'/' || c == L'\\' || c == L':'; }, L'_'
	);

	std::filesystem::path cachePath = m_cacheDirectory;
	cachePath /= safeName + L".dxc";
	return cachePath.wstring();
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
