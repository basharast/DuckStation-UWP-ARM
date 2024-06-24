// UWP STORAGE MANAGER
// Copyright (c) 2023 Bashar Astifan.
// Email: bashar@astifan.online
// Telegram: @basharastifan
// GitHub: https://github.com/basharast/UWP2Win32

// This code must keep support for lower builds (15063+)
// Try always to find possible way to keep that support

#pragma once 

#include "pch.h"
#include <io.h>
#include <fcntl.h>

#include "StorageLog.h"
#include "StoragePath.h"
#include "StorageExtensions.h"
#include "StorageHandler.h"
#include "StorageAsync.h"
#include "StorageInfo.h"

#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.FileProperties.h>
#include <winrt/Windows.Foundation.h>

using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Storage::FileProperties;

class StorageFileW {
public:
	StorageFileW() : storageFile(nullptr), properties(nullptr), fileSize(0) {
	}
	StorageFileW(StorageFile file) : storageFile(file), properties(nullptr), fileSize(0)
	{

	}

	StorageFileW(IStorageItem file) : storageFile(file.as<StorageFile>()), properties(nullptr), fileSize(0) {
	}

	~StorageFileW() {
	}

	// Detect if main storage file is not null
	bool IsValid() {
		return (storageFile != nullptr);
	}

	// Delete file
	bool Delete() {
		bool state = ExecuteTask(storageFile.DeleteAsync());
		if (state) {
			storageFile = nullptr;
		}
		return state;
	}

	// Rename file
	bool Rename(std::string name) {
		auto path = PathUWP(name);
		if (path.IsAbsolute()) {
			name = path.GetFilename();
		}
		return ExecuteTask(storageFile.RenameAsync(convert(name)));
	}

	// Copy file
	bool Copy(StorageFolder folder, std::string name) {
		bool state = false;
		StorageFile newFile(nullptr);
		ExecuteTask(newFile, storageFile.CopyAsync(folder, convert(name), NameCollisionOption::ReplaceExisting));
		if (newFile != nullptr) {
			state = true;
		}
		return state;
	}

	// Move file
	bool Move(StorageFolder folder, std::string name) {
		bool state = false;
		IStorageItem newFile;
		state = ExecuteTask(storageFile.MoveAsync(folder, convert(name), NameCollisionOption::GenerateUniqueName));
		if (state) {
			ExecuteTask(newFile, folder.TryGetItemAsync(storageFile.Name()));
			if (newFile != nullptr) {
				storageFile = newFile.as<StorageFile>();
			}
			else {
				state = false;
			}
		}
		return state;
	}

	// Get file path
	std::string GetPath() {
		return convert(storageFile.Path());
	}

	// Get file name
	std::string GetName() {
		return convert(storageFile.Name());
	}

	// Compare file with std::string
	bool Equal(std::string path) {
		std::string filePath = GetPath();

		// Fix slashs back from '/' to '\'
		windowsPath(path);
		return iequals(filePath, path);
	}

	// Compare file with Platform::String
	bool Equal(winrt::hstring path) {
		return storageFile.Path() == path;
	}

	// Compare file with Path
	bool Equal(PathUWP path) {
		return Equal(path.ToString());
	}

	// Compare file with StorageFile
	bool Equal(StorageFile file) {
		return Equal(file.Path());
	}

	// Get file size
	__int64 GetSize(bool updateCache = false) {
		if (fileSize == 0 || updateCache) {
			// Let's try getting size by handle first
			HANDLE handle;
			HRESULT hr = GetHandle(&handle);
			if (handle == INVALID_HANDLE_VALUE || hr != S_OK) {
				// We have no other option, fallback to UWP
				fileSize = FetchProperties().Size();
			}
			else {
				LARGE_INTEGER size{ 0 };
				if (FALSE == GetFileSizeEx(handle, &size)) {
					LARGE_INTEGER end_offset;
					const LARGE_INTEGER zero{};
					if (SetFilePointerEx(handle, zero, &end_offset, FILE_END) == 0) {
						CloseHandle(handle);
					}
					else {
						fileSize = end_offset.QuadPart;
						SetFilePointerEx(handle, zero, nullptr, FILE_BEGIN);
						CloseHandle(handle);
					}
				}
				else {
					fileSize = size.QuadPart;
					CloseHandle(handle);
				}
			}
		}
		return fileSize;
	}

	// Get file handle
	HRESULT GetHandle(HANDLE* handle, int accessMode = GENERIC_READ, int shareMode = FILE_SHARE_READ) {
		return GetFileHandle(storageFile, handle, GetAccessMode(accessMode), GetShareMode(shareMode));
	}

	// Get file stream
	FILE* GetStream(const char* mode) {
		HANDLE handle;
		auto fileMode = GetFileMode(mode);
		if (fileMode && !fileMode->isAppend && fileMode->isCreate) {
			ExecuteTask(winrt::Windows::Storage::FileIO::WriteTextAsync(storageFile, L""));
		}
		HRESULT hr = GetHandle(&handle, fileMode->dwDesiredAccess, fileMode->dwShareMode);
		FILE* file{};
		if (hr == S_OK && handle != INVALID_HANDLE_VALUE) {
			UWP_ERROR_LOG(UWPSMT, "Opening file (%s) with flag:%d mode:%s", GetPath().c_str(), fileMode->flags, mode);
			file = _fdopen(_open_osfhandle((intptr_t)handle, fileMode->flags), mode);
		}
		return file;
	}

	// Get file handle from stream
	HANDLE GetHandle(FILE* file) {
		return (HANDLE)_get_osfhandle(_fileno(file));
	}

	// Get file properties
	FILE_BASIC_INFO* GetProperties() {
		HANDLE handle;
		HRESULT hr = GetHandle(&handle);

		size_t size = sizeof(FILE_BASIC_INFO);
		FILE_BASIC_INFO* information = (FILE_BASIC_INFO*)(malloc(size));
		if (hr == S_OK && handle != INVALID_HANDLE_VALUE && information) {
			information->FileAttributes = (uint64_t)storageFile.Attributes();

			if (FALSE == GetFileInformationByHandleEx(handle, FileBasicInfo, information, (DWORD)size)) {
				// Fallback to UWP method (Slow)
				auto props = FetchProperties();
				information->ChangeTime.QuadPart = winrt::clock::to_file_time(props.DateModified()).value;
				information->CreationTime.QuadPart = winrt::clock::to_file_time(props.ItemDate()).value;
				information->LastAccessTime.QuadPart = winrt::clock::to_file_time(props.DateModified()).value;
				information->LastWriteTime.QuadPart = winrt::clock::to_file_time(props.DateModified()).value;
			}
			CloseHandle(handle);
		}

		return information;
	}

	// Get main storage file
	StorageFile GetStorageFile() {
		return storageFile;
	}

	time_t  filetime_to_timet(LARGE_INTEGER ull) const {
		return ull.QuadPart / 10000000ULL - 11644473600ULL;
	}
	ItemInfoUWP GetFileInfo() {
		ItemInfoUWP info;
		info.name = GetName();
		info.fullName = GetPath();
		info.isDirectory = false;

		auto sProperties = GetProperties();

		info.size = (uint64_t)GetSize();
		info.lastAccessTime = (uint64_t)filetime_to_timet(sProperties->LastAccessTime);
		info.lastWriteTime = (uint64_t)filetime_to_timet(sProperties->LastWriteTime);
		info.changeTime = (uint64_t)filetime_to_timet(sProperties->ChangeTime);
		info.creationTime = (uint64_t)filetime_to_timet(sProperties->CreationTime);


		info.attributes = (uint64_t)sProperties->FileAttributes;

		return info;
	}

private:
	StorageFile storageFile;
	BasicProperties properties;
	__int64 fileSize = 0;

	BasicProperties FetchProperties() {
		if (properties == nullptr) {
			// Very bad and slow way in UWP to get size and other properties
			// not preferred to be used on big list of files
			ExecuteTask(properties, storageFile.GetBasicPropertiesAsync());
		}
		return properties;
	}
};
