#pragma once

#include <Windows.h>
#include <string>

#define MEMORY_WIN32_IMPORTS 1

#if MEMORY_WIN32_IMPORTS

#pragma once

#include <windows.h>

LPVOID(WINAPI* VirtualAllocPtr)(LPVOID, SIZE_T, DWORD, DWORD) = VirtualAlloc;
BOOL(WINAPI* VirtualProtectPtr)(LPVOID, SIZE_T, ULONG, PULONG) = VirtualProtect;
HANDLE(WINAPI* CreateFileMappingWPtr)(HANDLE, LPSECURITY_ATTRIBUTES, DWORD, DWORD, DWORD, LPCWSTR) = CreateFileMappingW;
BOOL(WINAPI* UnmapViewOfFilePtr)(LPCVOID) = UnmapViewOfFile;
SIZE_T(WINAPI* VirtualQueryPtr)(LPCVOID, PMEMORY_BASIC_INFORMATION, SIZE_T) = VirtualQuery;
BOOL(WINAPI* FlushInstructionCachePtr)(HANDLE, LPCVOID, SIZE_T) = FlushInstructionCache;

HANDLE(WINAPI* CreateFile2Ptr)(LPCWSTR, DWORD, DWORD, DWORD, LPCREATEFILE2_EXTENDED_PARAMETERS) = CreateFile2;
HANDLE(WINAPI* FindFirstFileExWPtr)
(LPCWSTR, FINDEX_INFO_LEVELS, LPVOID, FINDEX_SEARCH_OPS, LPVOID, DWORD) = FindFirstFileExW;
BOOL(WINAPI* FindNextFileWPtr)(HANDLE, LPWIN32_FIND_DATAW) = FindNextFileW;
BOOL(WINAPI* GetFileAttributesExWPtr)(LPCWSTR, GET_FILEEX_INFO_LEVELS, LPVOID) = GetFileAttributesExW;
BOOL(WINAPI* DeleteFileWPtr)(LPCWSTR) = DeleteFileW;
BOOL(WINAPI* CreateDirectoryWPtr)(LPCWSTR, LPSECURITY_ATTRIBUTES) = CreateDirectoryW;
BOOL(WINAPI* CopyFileWPtr)(LPCWSTR, LPCWSTR, BOOL) = CopyFileW;
BOOL(WINAPI* MoveFileExWPtr)(LPCWSTR, LPCWSTR, DWORD) = MoveFileExW;
BOOL(WINAPI* RemoveDirectoryWPtr)(LPCWSTR) = RemoveDirectoryW;
DWORD(WINAPI* GetFileAttributesWPtr)(LPCWSTR) = GetFileAttributesW;

void LinkWin32APIs()
{
	// On Windows Phone (with root access Win32 can be used, and they are way faster)
	// in the code those are usually used when `g_settings.safe_memory` is false
	// Nothing to do here actually, but just incase
}

#endif