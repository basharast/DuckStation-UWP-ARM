// UWP STORAGE MANAGER
// Copyright (c) 2023 Bashar Astifan.
// Email: bashar@astifan.online
// Telegram: @basharastifan
// GitHub: https://github.com/basharast/UWP2Win32

#include "pch.h"

#include "StorageConfig.h"
#include "StorageManager.h"
#include "StorageExtensions.h"
#include "StorageHandler.h"
#include "StorageAsync.h"
#include "StorageAccess.h"
#include "StorageItemW.h"
#include "StorageLog.h"

#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Pickers.h>
#include <winrt/Windows.Storage.FileProperties.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <vector>

using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Storage::Pickers;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::ApplicationModel;
using namespace winrt::Windows::System;

extern std::list<StorageItemW> FutureAccessItems;

#pragma region Locations
std::string GetWorkingFolder() {
	if (AppWorkingFolder.empty()) {
		return GetLocalFolder();
	}
	else {
		return AppWorkingFolder;
	}
}
void SetWorkingFolder(std::string location) {
	AppWorkingFolder = location;
}
std::string GetInstallationFolder() {
	return convert(Package::Current().InstalledLocation().Path());
}
StorageFolder GetLocalStorageFolder() {
	return ApplicationData::Current().LocalFolder();
}
std::string GetLocalFolder() {
	return convert(GetLocalStorageFolder().Path());
}
std::string GetTempFolder() {
	return convert(ApplicationData::Current().TemporaryFolder().Path());
}
std::string GetTempFile(std::string name) {
	StorageFile tmpFile(nullptr);
	ExecuteTask(tmpFile, ApplicationData::Current().TemporaryFolder().CreateFileAsync(convert(name), CreationCollisionOption::GenerateUniqueName));
	if (tmpFile != nullptr) {
		return convert(tmpFile.Path());
	}
	else {
		return "";
	}
}
std::string GetPicturesFolder() {
	// Requires 'picturesLibrary' capability
	return convert(KnownFolders::PicturesLibrary().Path());
}
std::string GetVideosFolder() {
	// Requires 'videosLibrary' capability
	return convert(KnownFolders::VideosLibrary().Path());
}
std::string GetDocumentsFolder() {
	// Requires 'documentsLibrary' capability
	return convert(KnownFolders::DocumentsLibrary().Path());
}
std::string GetMusicFolder() {
	// Requires 'musicLibrary' capability
	return convert(KnownFolders::MusicLibrary().Path());
}
std::string GetPreviewPath(std::string path)
{
	std::string pathView = path;
	windowsPath(pathView);
	std::string appData = GetLocalFolder();
	replace(appData, "\\LocalState", "");
	replace(pathView, appData, "AppData");
	return pathView;
}
bool isLocalState(std::string path)
{
	return iequals(GetPreviewPath(path), "LocalState");
}
#pragma endregion

#pragma region Internal
PathUWP PathResolver(PathUWP path) {
	auto root = path.GetDirectory();
	auto newPath = path.ToString();
	if (path.IsRoot() || iequals(root, "/") || iequals(root, "\\")) {
		// System requesting file from app data
		replace(newPath, "/", (GetLocalFolder() + (path.size() > 1 ? "/" : "")));
	}
	path = PathUWP(newPath);
	return path;
}
PathUWP PathResolver(std::string path) {
	return PathResolver(PathUWP(path));
}
std::string ResolvePathUWP(std::string path) {
	return PathResolver(path).ToString();
}

// Return closer parent
StorageItemW GetStorageItemParent(PathUWP path) {
	path = PathResolver(path);
	StorageItemW parent;

	for (auto& fItem : FutureAccessItems) {
		if (isChild(fItem.GetPath(), path.ToString())) {
			if (fItem.IsDirectory()) {
				parent = fItem;
				break;
			}
		}
	}

	return parent;
}

StorageItemW GetStorageItem(PathUWP path, bool createIfNotExists = false, bool forceFolderType = false) {
	// Fill call will be ignored internally after the first call
	FillLookupList();

	path = PathResolver(path);
	StorageItemW item;

	// Look for match in FutureAccessItems
	for (auto& fItem : FutureAccessItems) {
		if (fItem.Equal(path)) {
			item = fItem;
			break;
		}
	}

	if (!item.IsValid()) {
		// Look for match inside FutureAccessFolders
		for (auto& fItem : FutureAccessItems) {
			if (fItem.IsDirectory()) {
				IStorageItem storageItem;
				if (fItem.Contains(path, storageItem)) {
					item = StorageItemW(storageItem);
					break;
				}
			}
		}
	}

	if (!item.IsValid() && createIfNotExists) {
		// Create and return new folder
		auto parent = GetStorageItemParent(path);
		if (parent.IsValid()) {
			if (!forceFolderType) {
				// File creation must be called in this case
				// Create folder usually will be called from 'CreateDirectory'
				item = StorageItemW(parent.CreateFile(path));
			}
			else {
				item = StorageItemW(parent.CreateFolder(path));
			}
		}
	}
	return item;
}

StorageItemW GetStorageItem(std::string path, bool createIfNotExists = false, bool forceFolderType = false) {
	return GetStorageItem(PathUWP(path), createIfNotExists, forceFolderType);
}

std::list<StorageItemW> GetStorageItemsByParent(PathUWP path) {
	path = PathResolver(path);
	std::list<StorageItemW> items;

	// Look for match in FutureAccessItems
	for (auto& fItem : FutureAccessItems) {
		if (isParent(path.ToString(), fItem.GetPath(), fItem.GetName())) {
			items.push_back(fItem);
		}
	}

	return items;
}

std::list<StorageItemW> GetStorageItemsByParent(std::string path) {
	return GetStorageItemsByParent(PathUWP(path));
}

bool IsContainsAccessibleItems(PathUWP path) {
	path = PathResolver(path);

	for (auto& fItem : FutureAccessItems) {
		if (isParent(path.ToString(), fItem.GetPath(), fItem.GetName())) {
			return true;
		}
	}

	return false;
}

bool IsContainsAccessibleItems(std::string path) {
	return IsContainsAccessibleItems(PathUWP(path));
}

bool IsRootForAccessibleItems(PathUWP path, std::list<std::string>& subRoot, bool breakOnFirstMatch = false) {
	path = PathResolver(path);

	for (auto& fItem : FutureAccessItems) {
		if (isChild(path.ToString(), fItem.GetPath())) {
			if (breakOnFirstMatch) {
				// Just checking, we don't need to loop for each item
				return true;
			}
			auto sub = getSubRoot(path.ToString(), fItem.GetPath());

			// This check can be better, but that's how I can do it in C++
			if (!ends_with(sub, ":")) {
				bool alreadyAdded = false;
				for (auto sItem : subRoot) {
					if (iequals(sItem, sub)) {
						alreadyAdded = true;
						break;
					}
				}
				if (!alreadyAdded) {
					subRoot.push_back(sub);
				}
			}
		}
	}
	return !subRoot.empty();
}

bool IsRootForAccessibleItems(std::string path, std::list<std::string>& subRoot, bool breakOnFirstMatch = false) {
	return IsRootForAccessibleItems(PathUWP(path), subRoot, breakOnFirstMatch);
}
bool IsRootForAccessibleItems(std::string path) {
	std::list<std::string> tmp;
	return IsRootForAccessibleItems(path, tmp, true);
}
#pragma endregion

#pragma region Functions
bool CreateIfNotExists(int openMode) {
	switch (openMode)
	{
	case CREATE_ALWAYS:
	case OPEN_ALWAYS:
	case CREATE_NEW:
		return true;
	default:
		return false;
	}
}

HANDLE CreateFileUWP(std::string path, long accessMode, long shareMode, long openMode) {
	HANDLE handle = INVALID_HANDLE_VALUE;
	if (IsValidUWP(path)) {
		bool createIfNotExists = CreateIfNotExists(openMode);
		auto storageItem = GetStorageItem(path, createIfNotExists);

		if (storageItem.IsValid()) {
			UWP_DEBUG_LOG(UWPSMT, "Getting handle (%s)", path.c_str());
			HRESULT hr = storageItem.GetHandle(&handle, accessMode, shareMode);
			if (hr == E_FAIL) {
				handle = INVALID_HANDLE_VALUE;
			}
		}
		else {
			handle = INVALID_HANDLE_VALUE;
			UWP_ERROR_LOG(UWPSMT, "Couldn't find or access (%s)", path.c_str());
		}
	}
	return handle;
}

HANDLE CreateFileUWP(std::wstring path, long accessMode, long shareMode, long openMode) {
	auto pathString = convert(path);
	return CreateFileUWP(pathString, accessMode, shareMode, openMode);
}

std::map<std::string, bool> accessState;
bool CheckDriveAccess(std::string driveName, bool checkIfContainsFutureAccessItems)
{
	bool state = false;

	auto keyIter = accessState.find(driveName);
	if (keyIter != accessState.end())
	{
		state = keyIter->second;
	}
	else
	{
		try {
			auto dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
			auto dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
			auto dwCreationDisposition = CREATE_ALWAYS;

			auto testFile = std::string(driveName);
			testFile.append("\\.UWPAccessCheck");
#if defined(_M_ARM)
			HANDLE h = CreateFile2(convertToLPCWSTR(testFile), dwDesiredAccess, dwShareMode, dwCreationDisposition, nullptr);
#else
			HANDLE h = CreateFile2FromAppW(convertToLPCWSTR(testFile), dwDesiredAccess, dwShareMode, dwCreationDisposition, nullptr);
#endif
			if (h != INVALID_HANDLE_VALUE) {
				state = true;
				CloseHandle(h);
#if defined(_M_ARM)
				DeleteFileW(convertToLPCWSTR(testFile));
#else
				DeleteFileFromAppW(convertToLPCWSTR(testFile));
#endif
			}
			accessState.insert(std::make_pair(driveName, state));
		}
		catch (...) {
		}
	}

	if (!state && checkIfContainsFutureAccessItems)
	{
		// Consider the drive accessible in case it contain files/folder selected before to avoid empty results
		state = IsRootForAccessibleItems(driveName) || IsContainsAccessibleItems(driveName);
	}
	return state;
}
bool IsValidUWP(std::string path, bool allowForAppData) {
	auto p = PathResolver(path);

	//Check valid path
	if (p.Type() == PathTypeUWP::UNDEFINED || !p.IsAbsolute()) {
		// Nothing to do here
		UWP_VERBOSE_LOG(UWPSMT, "File is not valid (%s)", p.ToString().c_str());
		return false;
	}

	bool state = false;

	if (!allowForAppData) {
		auto resolvedPathStr = p.ToString();
		if (ends_with(resolvedPathStr, "LocalState") || ends_with(resolvedPathStr, "TempState") ||
			ends_with(resolvedPathStr, "LocalCache"))
		{
			state = true;
		}
		else if (isChild(GetLocalFolder(), resolvedPathStr))
		{
			state = true;
		}
		else if (isChild(GetInstallationFolder(), resolvedPathStr))
		{
			state = true;
		}
		else if (isChild(GetTempFolder(), resolvedPathStr))
		{
			state = true;
		}

		if (!state)
		{
			auto p = PathUWP(path);
			std::string driveName = p.GetRootVolume().ToString();
			state = CheckDriveAccess(driveName, false);
		}
	}

	return !state;
}

bool IsExistsUWP(std::string path) {
	if (IsValidUWP(path)) {
		auto storageItem = GetStorageItem(path);
		if (storageItem.IsValid()) {
			return true;
		}

		// If folder is not accessible but contains accessible items
		// consider it exists
		if (IsContainsAccessibleItems(path)) {
			return true;
		}

		// If folder is not accessible but is part of accessible items
		// consider it exists
		std::list<std::string> tmp;
		if (IsRootForAccessibleItems(path, tmp, true)) {
			return true;
		}
	}
	// UWP_ERROR_LOG(UWPSMT, "Couldn't find or access (%s)", path.c_str());
	return false;
}
bool IsExistsUWP(std::wstring path)
{
	return IsExistsUWP(convert(path));
}

bool IsDirectoryUWP(std::string path) {
	if (IsValidUWP(path)) {
		auto storageItem = GetStorageItem(path);
		if (storageItem.IsValid()) {
			if (storageItem.IsDirectory()) {
				return true;
			}
		}
	}
	return false;
}

FILE* GetFileStream(std::string path, const char* mode) {
	FILE* file{};
	if (IsValidUWP(path)) {
		auto storageItem = GetStorageItem(path);
		if (storageItem.IsValid()) {
			file = storageItem.GetStream(mode);
		}
		else {
			// Forward the request to parent folder
			auto p = PathUWP(path);
			auto itemName = p.GetFilename();
			auto rootPath = p.GetDirectory();
			if (IsValidUWP(rootPath)) {
				storageItem = GetStorageItem(rootPath);
				if (storageItem.IsValid()) {
					file = storageItem.GetFileStream(itemName, mode);
				}
				else {
					UWP_ERROR_LOG(UWPSMT, "Couldn't find or access (%s)", rootPath.c_str());
					UWP_ERROR_LOG(UWPSMT, "Couldn't find or access (%s)", path.c_str());
				}
			}
		}
	}

	return file;
}

FILE* GetFileStreamFromApp(std::string path, const char* mode) {

	FILE* file{};

	auto pathResolved = PathUWP(ResolvePathUWP(path));
	HANDLE handle = INVALID_HANDLE_VALUE;

	auto fileMode = GetFileMode(mode);
	if (fileMode) {
		handle = CreateFile2(pathResolved.ToWString().c_str(), fileMode->dwDesiredAccess, fileMode->dwShareMode, fileMode->dwCreationDisposition, nullptr);
	}
	if (handle != INVALID_HANDLE_VALUE) {
		file = _fdopen(_open_osfhandle((intptr_t)handle, fileMode->flags), mode);
	}

	return file;
}

#pragma region Content Helpers
ItemInfoUWP GetFakeFolderInfo(std::string folder) {
	ItemInfoUWP info;
	auto folderPath = PathUWP(folder);
	info.name = folderPath.GetFilename();
	info.fullName = folderPath.ToString();

	info.isDirectory = true;

	info.size = 1;
	info.lastAccessTime = 1000;
	info.lastWriteTime = 1000;
	info.changeTime = 1000;
	info.creationTime = 1000;

	info.attributes = FILE_ATTRIBUTE_DIRECTORY;

	return info;
}

#pragma endregion

std::list<ItemInfoUWP> GetFolderContents(std::string path, bool deepScan) {
	std::list<ItemInfoUWP> contents;

	if (IsValidUWP(path)) {
		auto storageItem = GetStorageItem(path);
		if (storageItem.IsValid()) {

			// Files
			// deepScan is slow, try to avoid it
			auto rfiles = deepScan ? storageItem.GetAllFiles() : storageItem.GetFiles();
			for (auto file : rfiles) {
				contents.push_back(file.GetFileInfo());
			}

			// Folders
			// deepScan is slow, try to avoid it
			auto rfolders = deepScan ? storageItem.GetAllFolders() : storageItem.GetFolders();
			for (auto folder : rfolders) {
				contents.push_back(folder.GetFolderInfo());
			}
		}
		else {
			UWP_ERROR_LOG(UWPSMT, "Cannot get contents!, checking for other options.. (%s)", path.c_str());
		}
	}

	if (contents.size() == 0) {
		// Folder maybe not accessible or not exists
			// if not accessible, maybe some items inside it were selected before
			// and they already in our accessible list
		if (IsContainsAccessibleItems(path)) {
			UWP_DEBUG_LOG(UWPSMT, "Folder contains accessible items (%s)", path.c_str());

			// Check contents
			auto cItems = GetStorageItemsByParent(path);
			if (!cItems.empty()) {
				for (auto item : cItems) {
					UWP_VERBOSE_LOG(UWPSMT, "Appending accessible item (%s)", item.GetPath().c_str());
					contents.push_back(item.GetItemInfo());
				}
			}
		}
		else
		{
			// Check if this folder is root for accessible item
			// then add fake folder as sub root to avoid empty results
			std::list<std::string> subRoot;
			if (IsRootForAccessibleItems(path, subRoot)) {
				UWP_DEBUG_LOG(UWPSMT, "Folder is root for accessible items (%s)", path.c_str());

				if (!subRoot.empty()) {
					for (auto sItem : subRoot) {
						UWP_VERBOSE_LOG(UWPSMT, "Appending fake folder (%s)", sItem.c_str());
						contents.push_back(GetFakeFolderInfo(sItem));
					}
				}
			}
			else {
				UWP_ERROR_LOG(UWPSMT, "Cannot get any content!.. (%s)", path.c_str());
			}
		}
	}
	return contents;
}
std::list<ItemInfoUWP> GetFolderContents(std::wstring path, bool deepScan) {
	return GetFolderContents(convert(path), deepScan);
}

ItemInfoUWP GetItemInfoUWP(std::string path) {
	ItemInfoUWP info;
	info.size = -1;
	info.attributes = INVALID_FILE_ATTRIBUTES;

	if (IsValidUWP(path)) {
		auto storageItem = GetStorageItem(path);
		if (storageItem.IsValid()) {
			info = storageItem.GetItemInfo();
		}
		else {
			UWP_ERROR_LOG(UWPSMT, "Couldn't find or access (%s)", path.c_str());
		}
	}

	return info;
}
#pragma endregion

#pragma region Basics
int64_t GetSizeUWP(std::string path) {
	int64_t size = 0;
	if (IsValidUWP(path)) {
		auto storageItem = GetStorageItem(path);
		if (storageItem.IsValid()) {
			size = storageItem.GetSize();
		}
		else {
			UWP_ERROR_LOG(UWPSMT, "Couldn't find or access (%s)", path.c_str());
		}
	}
	return size;
}

bool DeleteUWP(std::string path) {
	bool state = false;
	if (IsValidUWP(path)) {
		auto storageItem = GetStorageItem(path);
		if (storageItem.IsValid()) {
			UWP_DEBUG_LOG(UWPSMT, "Delete (%s)", path.c_str());
			state = storageItem.Delete();
		}
		else {
			UWP_DEBUG_LOG(UWPSMT, "Couldn't find or access (%s)", path.c_str());
		}
	}

	return state;
}

bool DeleteUWP(std::wstring path)
{
	return DeleteUWP(convert(path));
}

bool CreateDirectoryUWP(std::string path, bool replaceExisting) {
	bool state = false;
	auto p = PathUWP(path);
	auto itemName = p.GetFilename();
	auto rootPath = p.GetDirectory();

	if (IsValidUWP(rootPath)) {
		auto storageItem = GetStorageItem(rootPath);
		if (storageItem.IsValid()) {
			UWP_DEBUG_LOG(UWPSMT, "Create new folder (%s)", path.c_str());
			state = storageItem.CreateFolder(itemName, replaceExisting);
		}
		else {
			UWP_ERROR_LOG(UWPSMT, "Couldn't find or access (%s)", rootPath.c_str());
		}
	}

	return state;
}

bool CopyUWP(std::string path, std::string dest) {
	bool state = false;

	if (IsValidUWP(path, true) && IsValidUWP(dest, true)) {
		auto srcStorageItem = GetStorageItem(path);
		if (srcStorageItem.IsValid()) {
			auto destDir = dest;
			auto srcName = srcStorageItem.GetName();
			auto dstPath = PathUWP(dest);
			auto dstName = dstPath.GetFilename();
			// Destination must be parent folder
			destDir = dstPath.GetDirectory();
			auto dstStorageItem = GetStorageItem(destDir, true, true);
			if (dstStorageItem.IsValid()) {
				UWP_DEBUG_LOG(UWPSMT, "Copy (%s) to (%s)", path.c_str(), dest.c_str());
				state = srcStorageItem.Copy(dstStorageItem, dstName);
			}
			else {
				UWP_ERROR_LOG(UWPSMT, "Couldn't find or access (%s)", dest.c_str());
			}
		}
		else {
			UWP_ERROR_LOG(UWPSMT, "Couldn't find or access (%s)", path.c_str());
		}
	}

	return state;
}

bool MoveUWP(std::string path, std::string dest) {
	bool state = false;

	if (IsValidUWP(path, true) && IsValidUWP(dest, true)) {
		auto srcStorageItem = GetStorageItem(path);

		if (srcStorageItem.IsValid()) {
			auto destDir = dest;
			auto srcName = srcStorageItem.GetName();
			auto dstPath = PathUWP(dest);
			auto dstName = dstPath.GetFilename();
			// Destination must be parent folder
			destDir = dstPath.GetDirectory();
			auto dstStorageItem = GetStorageItem(destDir, true, true);
			if (dstStorageItem.IsValid()) {
				UWP_DEBUG_LOG(UWPSMT, "Move (%s) to (%s)", path.c_str(), dest.c_str());
				state = srcStorageItem.Move(dstStorageItem, dstName);
			}
			else {
				UWP_ERROR_LOG(UWPSMT, "Couldn't find or access (%s)", dest.c_str());
			}
		}
		else {
			UWP_ERROR_LOG(UWPSMT, "Couldn't find or access (%s)", path.c_str());
		}
	}

	return state;
}

bool RenameUWP(std::string path, std::string name) {
	bool state = false;

	auto srcRoot = PathUWP(path).GetDirectory();
	auto dstRoot = PathUWP(name).GetDirectory();
	// Check if system using rename to move
	if (iequals(srcRoot, dstRoot)) {
		auto srcStorageItem = GetStorageItem(path);
		if (srcStorageItem.IsValid()) {
			UWP_DEBUG_LOG(UWPSMT, "Rename (%s) to (%s)", path.c_str(), name.c_str());
			state = srcStorageItem.Rename(name);
		}
		else {
			UWP_DEBUG_LOG(UWPSMT, "Couldn't find or access (%s)", path.c_str());
		}
	}
	else {
		UWP_DEBUG_LOG(UWPSMT, " Rename used as move -> call move (%s) to (%s)", path.c_str(), name.c_str());
		state = MoveUWP(path, name);
	}

	return state;
}

bool RenameUWP(std::wstring path, std::wstring name)
{
	return RenameUWP(convert(path), convert(name));
}
#pragma endregion


#pragma region Helpers
bool OpenFile(std::string path) {
	auto uri{ Uri(convert(path)) };

	bool state = false;
	auto storageItem = GetStorageItem(path);
	if (storageItem.IsValid())
	{
		if (!storageItem.IsDirectory())
		{
			ExecuteTask(state, Launcher::LaunchFileAsync(storageItem.GetStorageFile()), false);
		}
	}
	else
	{
		ExecuteTask(state, Launcher::LaunchUriAsync(uri), false);
	}
	return state;
}

bool OpenFolder(std::string path)
{
	bool state = false;
	winrt::hstring wString = convert(path);
	StorageFolder storageItem = nullptr;
	ExecuteTask(storageItem, StorageFolder::GetFolderFromPathAsync(wString));
	if (storageItem != nullptr)
	{
		ExecuteTask(state, winrt::Windows::System::Launcher::LaunchFolderAsync(storageItem), false);
	}
	else
	{
		// Try as it's file
		PathUWP parent = PathUWP(path);
		auto wParentString = hstring(parent.ToWString());

		ExecuteTask(storageItem, StorageFolder::GetFolderFromPathAsync(wParentString));
		if (storageItem != nullptr)
		{
			ExecuteTask(state, winrt::Windows::System::Launcher::LaunchFolderAsync(storageItem), false);
		}
	}
	return state;
}

bool GetDriveFreeSpace(PathUWP path, int64_t& space)
{
	bool state = false;
	winrt::hstring wString = winrt::hstring(path.ToWString().c_str());
	StorageFolder storageItem = nullptr;
	ExecuteTask(storageItem, StorageFolder::GetFolderFromPathAsync(wString));

	if (storageItem != nullptr) {
		hstring freeSpaceKey = hstring(L"System.FreeSpace");
		winrt::Windows::Foundation::Collections::IVector<hstring> propertiesToRetrieve{ winrt::single_threaded_vector<hstring>() };
		propertiesToRetrieve.Append(freeSpaceKey);
		winrt::Windows::Foundation::Collections::IMap<hstring, winrt::Windows::Foundation::IInspectable> result;
		ExecuteTask(result, storageItem.Properties().RetrievePropertiesAsync(propertiesToRetrieve));
		int64_t remainingSize = 0;
		if (result != nullptr && result.Size() > 0)
		{
			try
			{
				auto value = result.Lookup(L"System.FreeSpace");
				space = value.as<winrt::IPropertyValue>().GetUInt64();
				state = true;
			}
			catch (...)
			{
			}
		}
	}
	return state;
}

bool IsFirstStart() {
	auto firstrun = GetDataFromLocalSettings("first_run");
	AddDataToLocalSettings("first_run", "done", true);
	return firstrun.empty();
}
#pragma endregion

#pragma region Logs
// Get log file name
std::string currentLogFile;
std::string getLogFileName() {
	//Initial new name each session/launch
	if (currentLogFile.empty() || currentLogFile.size() == 0) {
		std::time_t now = std::time(0);
		char mbstr[100];
		std::strftime(mbstr, 100, "ppsspp %d-%m-%Y (%T).txt", std::localtime(&now));
		std::string formatedDate(mbstr);
		std::replace(formatedDate.begin(), formatedDate.end(), ':', '-');
		currentLogFile = formatedDate;
	}

	return currentLogFile;
}

// Get current log file location
StorageFolder GetLogsStorageFolder() {
	// Ensure 'LOGS' folder is created
	auto workingFolder = GetStorageItem(GetWorkingFolder());
	StorageFolder logsFolder(nullptr);
	if (workingFolder.IsValid()) {
		auto workingStorageFolder = workingFolder.GetStorageFolder();
		ExecuteTask(logsFolder, workingStorageFolder.CreateFolderAsync(L"LOGS", CreationCollisionOption::OpenIfExists));
	}
	return logsFolder;
}
std::string GetLogFile() {
	std::string logFilePath = "";

	// Ensure 'LOGS' folder is created
	StorageFolder logsFolder = GetLogsStorageFolder();

	if (logsFolder != nullptr) {
		auto logFileName = convert(getLogFileName());
		StorageFile logFile(nullptr);
		ExecuteTask(logFile, logsFolder.CreateFileAsync(logFileName, CreationCollisionOption::OpenIfExists));

		if (logFile != nullptr) {
			logFilePath = convert(logFile.Path());
		}
	}

	return logFilePath;
}

// Save logs to folder selected by the user
bool SaveLogs() {
	try {
		auto folderPicker{ Pickers::FolderPicker() };
		folderPicker.SuggestedStartLocation(Pickers::PickerLocationId::Desktop);
		folderPicker.FileTypeFilter().Append(L"*");

		StorageFolder saveFolder(nullptr);
		ExecuteTask(saveFolder, folderPicker.PickSingleFolderAsync());

		if (saveFolder != nullptr) {
			StorageFolder logsFolder = GetLogsStorageFolder();

			if (logsFolder != nullptr) {
				StorageFolderW logsCache(logsFolder);
				logsCache.Copy(saveFolder);
			}
		}
	}
	catch (...) {
		return false;
	}
	return true;
}

void CleanupLogs() {
	StorageFolder logsFolder = GetLogsStorageFolder();
	if (logsFolder != nullptr) {
		StorageFolderW logsCache(logsFolder);
		std::list<StorageFileW> files = logsCache.GetFiles();
		if (!files.empty()) {
			for (auto fItem : files) {
				if (fItem.GetSize() == 0) {
					fItem.Delete();
				}
			}
		}
	}
}
#pragma endregion
