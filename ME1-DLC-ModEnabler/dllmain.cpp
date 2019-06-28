// dllmain.cpp : Defines the entry point for the DLL application.
#include <windows.h>
#include <vector>
#include <filesystem>
#include "Shlobj.h"
#include <string>
#include "IniFile.h"
#include "FileIndexes.h"

namespace fs = std::filesystem;

#pragma pack(1)

//#define LOGGING

//using namespace std;

typedef unsigned short ushort;

#ifdef LOGGING
std::wofstream logFile;
#endif // LOGGING

void addToMap(std::map<fs::path, std::wstring>& fileMap, const fs::path& dlcPath)
{
#ifdef LOGGING
	logFile << '\n';
	logFile << "************************\n";
	logFile << dlcPath.filename() << '\n';
	logFile << "************************\n";
#endif // LOGGING
	const auto cookedPath = dlcPath / "CookedPC";
	if (is_directory(cookedPath))
	{
		const auto dlcName = dlcPath.filename().wstring();
		for (auto& entry : fs::recursive_directory_iterator(cookedPath))
		{
			if (entry.is_regular_file())
			{
				fileMap[relative(entry.path(), cookedPath)] = dlcName;
			}
		}
	}
}

fs::path GetDocumentsFolder()
{
	PWSTR docPathPtr = nullptr;
	SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &docPathPtr);
	const std::wstring docPath(docPathPtr);
	CoTaskMemFree(docPathPtr);
	return fs::path(docPath);
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

		std::map<fs::path, std::wstring> fileMap;
		FileIndexes fileIndexes;
		std::vector<std::wstring> dlcWithMovies;
		std::map<int, fs::path> dlcMount;
		std::map<int, std::wstring> dlcFriendlyNames;
		//basegame
		const auto biogameDir = baseDir / "BioGame";
		addToMap(fileMap, biogameDir);
		fileIndexes.newFile(L"BioGame", biogameDir);

		const auto dlcDir = baseDir / L"DLC";
		auto bdtsName = L"DLC_UNC";
		const auto bdtsDir = dlcDir / bdtsName;
		if (is_directory(bdtsDir))
		{
			dlcMount[1] = bdtsDir;
			dlcFriendlyNames[1] = L"Bring Down the Sky";
		}
		auto pinnacleName = L"DLC_Vegas";
		const auto pinnacleDir = dlcDir / pinnacleName;
		if (is_directory(pinnacleDir))
		{
			dlcMount[2] = pinnacleDir;
			dlcFriendlyNames[2] = L"Pinnacle Station";
		}

		//get load order
		for (auto& entry : fs::directory_iterator(dlcDir))
		{
			if (entry.is_directory())
			{
				const auto& dlc_path = entry.path();
				auto dlcName = dlc_path.filename().wstring();
				if (dlcName != bdtsName && dlcName != pinnacleName)
				{
					auto autoLoadPath = dlc_path / L"AutoLoad.ini";
					if (is_regular_file(autoLoadPath))
					{
						const IniFile autoLoad(autoLoadPath);
						auto strMount = autoLoad.readValue(L"ME1DLCMOUNT", L"ModMount");
						if (!strMount.empty())
						{
							try
							{
								auto mount = std::stoi(strMount);
								dlcMount[mount] = dlc_path;
								dlcFriendlyNames[mount] = autoLoad.readValue(L"ME1DLCMOUNT", L"ModName");
							}
							catch (...)
							{
								//TODO
							}
							continue;
						}
						//TODO
					}
					//TODO
				}
			}
		}

		fs::path bioEnginePath = GetDocumentsFolder() / L"BioWare" / L"Mass Effect" / L"Config" / L"BIOEngine.ini";

#ifdef LOGGING
		logFile << "BIOEngine.ini path:\n";
		logFile << bioEnginePath << '\n';
#endif // LOGGING
		if (is_regular_file(bioEnginePath))
		{
			IniFile BIOEngine(bioEnginePath);

			auto section = L"Core.System";
			auto moviePathKey = L"DLC_MoviePaths";
			auto seekFreeKey = L"SeekFreePCPaths";
			BIOEngine.removeKeys(section, seekFreeKey);
			BIOEngine.removeKeys(section, moviePathKey);
			BIOEngine.writeNewValue(section, seekFreeKey, L"..\\BioGame\\CookedPC");

			std::wofstream loadOrderTxt(dlcDir / L"LoadOrder.Txt");
			loadOrderTxt << L"This is an auto-generated file for informational purposes only. Editing it will not change the load order.\n\n";
			for (auto&& pair : dlcMount)
			{
				fs::path dlcPath = pair.second;
				std::wstring dlcName = dlcPath.filename().wstring();

				addToMap(fileMap, dlcPath);
				fileIndexes.newFile(dlcName, dlcPath);
				if (is_directory(dlcPath / "Movies"))
				{
#ifdef LOGGING
					logFile << "has Movies\n";
#endif // LOGGING
					dlcWithMovies.push_back(dlcName);
				}
				BIOEngine.writeNewValue(section, seekFreeKey, L"..\\DLC\\" + dlcName + L"\\CookedPC");


				loadOrderTxt << pair.first << L": " << dlcName << L" (" << dlcFriendlyNames[pair.first] << L")" << std::endl;
			}
			loadOrderTxt.close();

			fileIndexes.write(fileMap);

			for (auto&& dlcName : dlcWithMovies)
			{
				BIOEngine.writeNewValue(section, moviePathKey, L"..\\DLC\\" + dlcName + L"\\Movies");
			}

#ifdef LOGGING
			logFile << "{\n";
			for (const auto& p : fileMap)
				logFile << p.first << ':' << p.second << ",\n";
			logFile << "}\n" << std::endl;

			logFile << "writing all FileIndex.txt\n";
#endif // LOGGING
		}
		else
		{
			//TODO
		}

#ifdef LOGGING
		logFile << "done\n";
		logFile.close();
#endif // LOGGING
	}

	return TRUE;
}
