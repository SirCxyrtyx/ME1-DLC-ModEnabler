// dllmain.cpp : Defines the entry point for the DLL application.
#include <windows.h>
#include <vector>
#include <map>
#include <fstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

#pragma pack(1)

//#define LOGGING

//using namespace std;

typedef unsigned short ushort;

#ifdef LOGGING
std::ofstream logFile;
#endif // LOGGING

void addToMap(std::map<fs::path, std::string>& fileMap, const fs::path& dlcPath)
{
#ifdef LOGGING
	logFile << std::endl;
	logFile << "************************\n";
	logFile << "************************\n";
	logFile << dlcPath.filename() << '\n';
	logFile << "************************\n";
	logFile << "************************\n";
	logFile << std::endl;
#endif // LOGGING
	const auto cookedPath = dlcPath / "CookedPC";
	if (is_directory(cookedPath))
	{
		const auto dlcName = dlcPath.filename().string();
		for (auto& entry : fs::recursive_directory_iterator(cookedPath))
		{
			if (entry.is_regular_file())
			{
				fileMap[relative(entry.path(), cookedPath)] = dlcName;
			}
		}
	}
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD  reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{

		TCHAR modulePath[MAX_PATH];
		GetModuleFileName(hModule, modulePath, MAX_PATH);
		const auto asiDirPath = fs::path(modulePath).parent_path();

#ifdef LOGGING
		logFile.open(asiDirPath / "ME1_DLC_ModEnabler.log");
		logFile << "ME1_DLC_ModEnabler log:\n";
#endif // LOGGING

		const auto baseDir = asiDirPath.parent_path().parent_path();

#ifdef LOGGING
		logFile << "making file map...\n";
#endif // LOGGING

		std::map<fs::path, std::string> fileMap;
		std::map<std::string, std::ofstream*> outputFiles;
		const auto suffix = fs::path("CookedPC") / "FileIndex.txt";
		//basegame
		const auto biogameDir = baseDir / "BioGame";
		addToMap(fileMap, biogameDir);
		outputFiles["BioGame"] = new std::ofstream(biogameDir / suffix);

		const auto dlcDir = baseDir / "DLC";
		const auto bdtsDir = dlcDir / "DLC_UNC";
		if (is_directory(bdtsDir))
		{
			addToMap(fileMap, bdtsDir);
			outputFiles["DLC_UNC"] = new std::ofstream(bdtsDir / suffix);
		}
		const auto pinnacleDir = dlcDir / "DLC_Vegas";
		if (is_directory(pinnacleDir))
		{
			addToMap(fileMap, pinnacleDir);
			outputFiles["DLC_Vegas"] = new std::ofstream(pinnacleDir / suffix);
		}

		//mods
		for (auto& entry : fs::directory_iterator(dlcDir))
		{
			if (entry.is_directory())
			{
				const auto& dlc_path = entry.path();
				auto dlcName = dlc_path.filename().string();
				if (dlcName != "DLC_UNC" && dlcName != "DLC_Vegas")
				{
					addToMap(fileMap, dlc_path);
					outputFiles[dlcName] = new std::ofstream(dlc_path / suffix);
				}
			}
		}

#ifdef LOGGING
		logFile << "{\n";
		for (const auto& p : fileMap)
			logFile << p.first << ':' << p.second << ",\n";
		logFile << "}\n" << std::endl;

		logFile << "writing all FileIndex.txt\n";
#endif // LOGGING

		for (auto && pair : fileMap)
		{
			*(outputFiles[pair.second]) << pair.first.string() << '\n';
		}

		for (auto && pair : outputFiles)
		{
			pair.second->close();
			delete pair.second;
		}

#ifdef LOGGING
		logFile << "done\n";
		logFile.close();
#endif // LOGGING
	}

	return TRUE;
}
