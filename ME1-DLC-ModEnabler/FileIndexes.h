#pragma once
#include <map>
#include <string>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class FileIndexes
{
	inline static const fs::path suffix = fs::path(L"CookedPC") / L"FileIndex.txt";

	std::map<std::wstring, std::wofstream*> outputFiles;
public:
	~FileIndexes();

	void newFile(const std::wstring& name, const fs::path& path);

	void write(const std::map<fs::path, std::wstring>& fileMap);
};
