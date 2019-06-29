// dllmain.cpp : Defines the entry point for the DLL application.
#include <windows.h>
#include <vector>
#include <filesystem>
#include "Shlobj.h"
#include <string>
#include "IniFile.h"
#include "FileIndexes.h"
#include <map>
#ifdef LOGGING
#include <chrono>
#endif // LOGGING

namespace fs = std::filesystem;

#pragma pack(1)

//#define LOGGING

//using namespace std;

typedef unsigned short ushort;

#ifdef LOGGING
std::wofstream logFile;
#endif // LOGGING

void addToMap(std::map<std::wstring, std::wstring>& fileMap, const fs::path& dlcPath)
{
#ifdef LOGGING
	logFile << '\n';
	logFile << "************************\n";
	logFile << dlcPath.filename() << '\n';
	logFile << "************************\n";
#endif // LOGGING
	const auto cookedPath = dlcPath / "CookedPC";
	const auto cookedLen = cookedPath.wstring().length() + 1;
	if (is_directory(cookedPath))
	{
		const auto dlcName = dlcPath.filename().wstring();
		for (auto& entry : fs::recursive_directory_iterator(cookedPath))
		{
			if (entry.is_regular_file())
			{
				fileMap[entry.path().wstring().substr(cookedLen)] = dlcName;
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

void bookkeeping(const fs::path& baseDir)
{
#ifdef LOGGING
	const auto startTime = std::chrono::high_resolution_clock::now();
#endif // LOGGING
#ifdef LOGGING
	logFile << "making file map...\n";
#endif // LOGGING

	std::map<std::wstring, std::wstring> fileMap;
	FileIndexes fileIndexes;
	std::vector<std::wstring> dlcWithMovies;
	std::map<int, fs::path> dlcMount;
	std::map<int, std::wstring> dlcFriendlyNames;

#ifdef LOGGING
	logFile << L"Before mapping basegame: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime).count() << '\n';
#endif // LOGGING
	const auto biogame = std::wstring(L"BioGame");
	//basegame
	const auto biogameDir = baseDir / biogame;
	addToMap(fileMap, biogameDir);
	fileIndexes.newFile(biogame, biogameDir);

#ifdef LOGGING
	logFile << L"Before getting load order: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime).count() << '\n';
#endif // LOGGING

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
						int mount = 0;
						try
						{
							mount = std::stoi(strMount);
						}
						catch (...)
						{
							mount = 0;
						}
						if (mount > 2)
						{
							dlcMount[mount] = dlc_path;
							dlcFriendlyNames[mount] = autoLoad.readValue(L"ME1DLCMOUNT", L"ModName");
							continue;
						}
					}
					const std::wstring message = L" has an improperly formatted AutoLoad.ini! Try re-installing the mod. If that doesn't fix the problem, contact the mod author.\n\nMass Effect will now close.";
					MessageBox(nullptr,
					           (dlcName + message).c_str(),
					           L"Mass Effect - Broken Mod Warning",
					           MB_OK | MB_ICONERROR);
					exit(1);
				}
				//no AutoLoad.ini! ME1 will not attempt to load this folder as a DLC, so we can ignore it.
			}
		}
	}
#ifdef LOGGING
	logFile << L"Time to read load order: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime).count() << '\n';
#endif // LOGGING

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


#ifdef LOGGING
		logFile << L"Time before writing FileIndex.txt: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime).count() << '\n';
#endif // LOGGING

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
		MessageBox(nullptr,
		           L"No BIOEngine.ini found! Once you reach the main menu, please restart the game to complete initialization",
		           L"Mass Effect - First Time Setup Warning",
		           MB_OK | MB_ICONWARNING);
	}
#ifdef LOGGING
	logFile << L"Elapsed Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime).count() << '\n';
#endif // LOGGING
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
#ifdef LOGGING
		const auto startTime = std::chrono::high_resolution_clock::now();
#endif // LOGGING

		TCHAR modulePath[MAX_PATH];
		GetModuleFileName(hModule, modulePath, MAX_PATH);
		const auto asiDirPath = fs::path(modulePath).parent_path();

#ifdef LOGGING
		logFile.open(asiDirPath / "ME1_DLC_ModEnabler.log");
		logFile << "ME1_DLC_ModEnabler log:\n";
#endif // LOGGING

		bookkeeping(asiDirPath.parent_path().parent_path());

#ifdef LOGGING
		const auto elapsedDuration = std::chrono::high_resolution_clock::now() - startTime;

		logFile << L"Elapsed Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(elapsedDuration).count() << '\n';

		logFile << "done\n";
		logFile.close();
#endif // LOGGING
	}

	return TRUE;
}
