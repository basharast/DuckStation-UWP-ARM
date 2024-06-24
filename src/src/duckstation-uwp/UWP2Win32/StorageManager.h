// UWP STORAGE MANAGER
// Copyright (c) 2023 Bashar Astifan.
// Email: bashar@astifan.online
// Telegram: @basharastifan
// GitHub: https://github.com/basharast/UWP2Win32

// This code must keep support for lower builds (15063+)
// Try always to find possible way to keep that support

#pragma once 

#include <list>

#include "StoragePath.h"
#include "StorageInfo.h"
#include "StorageAccess.h"
#include "StoragePickers.h"

// Locations
std::string GetWorkingFolder(); // Where main data is, default is app data
void SetWorkingFolder(std::string location); // Change working location
std::string GetInstallationFolder();
std::string GetLocalFolder();
std::string GetTempFolder();
std::string GetTempFile(std::string name);
std::string GetPicturesFolder(); // Requires 'picturesLibrary' capability
std::string GetVideosFolder(); // Requires 'videosLibrary' capability
std::string GetDocumentsFolder(); // Requires 'documentsLibrary' capability
std::string GetMusicFolder(); // Requires 'musicLibrary' capability
std::string GetPreviewPath(std::string path);
bool isLocalState(std::string path);

// Management
HANDLE CreateFileUWP(std::string path, long accessMode = 0x80000000L, long shareMode = 0x00000001, long openMode = 3);
HANDLE CreateFileUWP(std::wstring path, long accessMode = 0x80000000L, long shareMode = 0x00000001, long openMode = 3);
FILE* GetFileStream(std::string path, const char* mode);
bool IsValidUWP(std::string path, bool allowForAppData = false);
bool IsExistsUWP(std::string path);
bool IsExistsUWP(std::wstring path);
bool IsDirectoryUWP(std::string path);

std::list<ItemInfoUWP> GetFolderContents(std::string path, bool deepScan = false);
std::list<ItemInfoUWP> GetFolderContents(std::wstring path, bool deepScan = false);
ItemInfoUWP GetItemInfoUWP(std::string path);

// Basics
int64_t GetSizeUWP(std::string path);
bool DeleteUWP(std::string path);
bool DeleteUWP(std::wstring path);
bool CreateDirectoryUWP(std::string path, bool replaceExisting = true);
bool RenameUWP(std::string path, std::string name);
bool RenameUWP(std::wstring path, std::wstring name);
// Add file name to destination path
bool CopyUWP(std::string path, std::string dest);
// Add file name to destination path
bool MoveUWP(std::string path, std::string dest);

// Helpers
bool OpenFile(std::string path);
bool OpenFolder(std::string path);
bool IsFirstStart();
std::string ResolvePathUWP(std::string path);
bool IsContainsAccessibleItems(std::string path);
bool IsRootForAccessibleItems(std::string path);
// 'checkIfContainsFutureAccessItems' for listing purposes not real access, 'driveName' like C:
bool CheckDriveAccess(std::string driveName, bool checkIfContainsFutureAccessItems);
bool GetDriveFreeSpace(PathUWP path, int64_t& space);

// Log helpers
std::string GetLogFile();
bool SaveLogs(); // With picker
void CleanupLogs();

