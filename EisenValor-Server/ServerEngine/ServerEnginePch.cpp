#include "pch.h"
#include "ServerEnginePch.h"

std::string WStringToString(const std::wstring_view wstr)
{
	std::filesystem::path p(wstr.data());
	return p.string();
}

std::wstring StringToWString(const std::string_view str)
{
	std::filesystem::path p(str.data());
	return p.wstring();
}



