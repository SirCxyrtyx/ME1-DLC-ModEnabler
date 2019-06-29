#include "FileIndexes.h"

FileIndexes::~FileIndexes()
{
	for (auto&& pair : outputFiles)
	{
		pair.second->close();
		delete pair.second;
	}
}

 void FileIndexes::newFile(const std::wstring& name, const fs::path& path)
{
	outputFiles[name] = new std::wofstream(path / suffix);
}

void FileIndexes::write(const std::map<std::wstring, std::wstring>& fileMap)
{
	for (auto&& pair : fileMap)
	{
		*(outputFiles[pair.second]) << pair.first << '\n';
	}
}