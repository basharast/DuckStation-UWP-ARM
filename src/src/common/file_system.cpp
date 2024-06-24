/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2021  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "file_system.h"
#include "assert.h"
#include "log.h"
#include "path.h"
#include "string_util.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <limits>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <stdlib.h>
#include <sys/param.h>
#endif

#ifdef __FreeBSD__
#include <sys/sysctl.h>
#endif

#if defined(_WIN32)
#include "windows_headers.h"
#include <share.h>
#include <shlobj.h>
#include <winioctl.h>

#if defined(_UWP)
#include "duckstation-uwp/UWP2Win32/StorageHandler.h"
#include <fcntl.h>
#include <io.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.AccessCache.h>
#include <winrt/Windows.Storage.FileProperties.h>
#include <winrt/Windows.Storage.Pickers.h>
#include <winrt/Windows.Storage.Search.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.UI.Core.h>
#endif

#else
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#include "core/host_interface.h"
#include "frontend-common/fullscreen_ui.h"
#include <StoragePath.h>
#include <duckstation-uwp/UWP2Win32/StorageExtensions.h>

Log_SetChannel(FileSystem);

extern HANDLE(WINAPI* CreateFile2Ptr)(LPCWSTR, DWORD, DWORD, DWORD, LPCREATEFILE2_EXTENDED_PARAMETERS);
extern HANDLE(WINAPI* FindFirstFileExWPtr)(LPCWSTR, FINDEX_INFO_LEVELS, LPVOID, FINDEX_SEARCH_OPS, LPVOID, DWORD);
extern BOOL(WINAPI* FindNextFileWPtr)(HANDLE, LPWIN32_FIND_DATAW);
extern BOOL(WINAPI* GetFileAttributesExWPtr)(LPCWSTR, GET_FILEEX_INFO_LEVELS, LPVOID);
extern BOOL(WINAPI* DeleteFileWPtr)(LPCWSTR);
extern BOOL(WINAPI* CreateDirectoryWPtr)(LPCWSTR, LPSECURITY_ATTRIBUTES);
extern BOOL(WINAPI* CopyFileWPtr)(LPCWSTR, LPCWSTR, BOOL);
extern BOOL(WINAPI* MoveFileExWPtr)(LPCWSTR, LPCWSTR, DWORD);
extern BOOL(WINAPI* RemoveDirectoryWPtr)(LPCWSTR);
extern DWORD(WINAPI* GetFileAttributesWPtr)(LPCWSTR);
extern winrt::Windows::Storage::StorageFolder dataFolder;

// Get value from app local settings
winrt::hstring GetDataFromLocalSettings(winrt::hstring key)
{
  winrt::Windows::Storage::ApplicationDataContainer localSettings{
    winrt::Windows::Storage::ApplicationData::Current().LocalSettings()};
  winrt::Windows::Foundation::Collections::IPropertySet values{localSettings.Values()};
  if (!key.empty())
  {
    auto tokenRetrive = values.Lookup(key);
    if (tokenRetrive != nullptr)
    {
      winrt::hstring ConvertedToken = tokenRetrive.as<winrt::Windows::Foundation::IPropertyValue>().GetString();
      return ConvertedToken;
    }
  }
  return {};
}

std::string GetDataFromLocalSettings(std::string key)
{
  return convert(GetDataFromLocalSettings(convert(key)));
}

// Add or replace value in app local settings
bool AddDataToLocalSettings(winrt::hstring key, winrt::hstring data, bool replace)
{
  winrt::Windows::Storage::ApplicationDataContainer localSettings{
    winrt::Windows::Storage::ApplicationData::Current().LocalSettings()};
  winrt::Windows::Foundation::Collections::IPropertySet values{localSettings.Values()};

  winrt::hstring testResult = GetDataFromLocalSettings(key);
  if (testResult.empty())
  {
    values.Insert(key, winrt::Windows::Foundation::PropertyValue::CreateString(data));
    return true;
  }
  else if (replace)
  {
    values.Remove(key);
    values.Insert(key, winrt::Windows::Foundation::PropertyValue::CreateString(data));
    return true;
  }

  return false;
}

bool AddDataToLocalSettings(std::string key, std::string data, bool replace)
{
  return AddDataToLocalSettings(convert(key), convert(data), replace);
}

winrt::Windows::Storage::StorageFolder GetLocalStorageFolder()
{
  return winrt::Windows::Storage::ApplicationData::Current().LocalFolder();
}
std::string GetLocalFolder()
{
  return convert(GetLocalStorageFolder().Path());
}

std::string GetPreviewPath(std::string path)
{
  std::string pathView = path;
  windowsPath(pathView);

  // Ensure memStick sub path replaced by 'ms:'
  std::string appData = GetLocalFolder();
  replace(appData, "\\LocalState", "");
  replace(pathView, appData, "AppData");
  return pathView;
}

// Parent and child full path
std::string getSubRoot2(std::string parent, std::string child)
{
  auto childCut = child;
  childCut = replace(childCut, (parent + "\\"), "");
  size_t len = childCut.find_first_of('\\', 0);
  auto subRoot = childCut.substr(0, len);

  return parent + "\\" + subRoot;
}

bool isChild2(std::string parent, std::string child)
{
  return child.find(parent) != std::string::npos;
}

// Parent full path, child full path, child name only
bool isParent2(std::string parent, std::string child, std::string childName)
{
  parent.append("\\" + childName);
  return parent == child;
}

std::list<std::string> alist;
std::list<std::string> aflist;
void AppendToAccessList(winrt::hstring path, bool isFolder)
{
  PathUWP p(convert(path));
  bool appendItem = true;
  for (auto sItem = alist.begin(); sItem != alist.end(); ++sItem)
  {
    if (std::string(sItem->c_str()) == p.ToString())
    {
      appendItem = false;
      break;
    }
  }
  if (appendItem)
  {
    alist.push_back(p.ToString());
  }

  if (isFolder)
  {
    bool appendItemf = true;
    for (auto sItem = aflist.begin(); sItem != aflist.end(); ++sItem)
    {
      if (std::string(sItem->c_str()) == p.ToString())
      {
        appendItemf = false;
        break;
      }
    }
    if (appendItemf)
    {
      aflist.push_back(p.ToString());
    }
  }
}

// Add folder to future list (to avoid request picker again)
void AddItemToFutureList(winrt::Windows::Storage::IStorageItem item)
{
  try
  {
    if (item != nullptr)
    {
      winrt::hstring folderToken =
        winrt::Windows::Storage::AccessCache::StorageApplicationPermissions::FutureAccessList().Add(item);
      bool isFolder = (item.IsOfType(winrt::Windows::Storage::StorageItemTypes::Folder));
      {
        AppendToAccessList(item.Path(), isFolder);
      }
    }
  }
  catch (...)
  {
  }
}

std::string getCurrentStorage()
{
  std::string currentStorage = "";
  if (convert(dataFolder.Path()) != GetLocalFolder())
  {
    currentStorage = convert(dataFolder.Path());
  }

  return currentStorage;
}

std::list<std::string> GetFutureAccessList()
{
  if (alist.empty())
  {
    AppendToAccessList(winrt::Windows::Storage::ApplicationData::Current().LocalFolder().Path(), true);

    auto AccessList = winrt::Windows::Storage::AccessCache::StorageApplicationPermissions::FutureAccessList().Entries();
    for (auto it = 0; it != AccessList.Size(); ++it)
    {
      auto item = AccessList.GetAt(it);
      try
      {
        auto token = item.Token;
        if (!token.empty() &&
            winrt::Windows::Storage::AccessCache::StorageApplicationPermissions::FutureAccessList().ContainsItem(token))
        {
          winrt::Windows::Storage::IStorageItem storageItem =
            winrt::Windows::Storage::AccessCache::StorageApplicationPermissions::FutureAccessList()
              .GetItemAsync(token)
              .get();

          if (storageItem != nullptr)
          {
            bool isFolder = (storageItem.IsOfType(winrt::Windows::Storage::StorageItemTypes::Folder));
            {
              AppendToAccessList(storageItem.Path(), isFolder);
            }
          }
          else
          {
            winrt::Windows::Storage::AccessCache::StorageApplicationPermissions::FutureAccessList().Remove(token);
          }
        }
      }
      catch (...)
      {
      }
    }
  }
  return alist;
}

std::string GetFolderParent(const std::string& str)
{
  size_t found;
  found = str.find_last_of("/\\");
  return str.substr(0, found);
}

std::string GetFolderName(const std::string& str)
{
  size_t found;
  found = str.find_last_of("/\\");
  return str.substr(found + 1);
}

PathUWP PathResolver(PathUWP path)
{
  auto root = path.GetDirectory();
  auto newPath = path.ToString();
  if (path.IsRoot() || !_stricmp(root.c_str(), "\\"))
  {
    // System requesting file from app data
    newPath = replace(newPath, "\\", (GetLocalFolder() + (path.size() > 1 ? "\\" : "")));
  }
  return PathUWP(newPath);
}
PathUWP PathResolver(std::string path)
{
  return PathResolver(PathUWP(path));
}

bool IsRootForAccessibleItems(PathUWP path, std::list<std::string>& subRoot, bool breakOnFirstMatch = false)
{
  // path = PathResolver(path);
  auto FutureAccessItems = GetFutureAccessList();
  for (auto& fItem : FutureAccessItems)
  {
    if (isChild(path.ToString(), fItem))
    {
      if (breakOnFirstMatch)
      {
        // Just checking, we don't need to loop for each item
        return true;
      }
      auto sub = getSubRoot(path.ToString(), fItem);
      rtrim(sub, "\\");
      // This check can be better, but that's how I can do it in C++
      if (!ends_with(sub, ":"))
      {
        bool alreadyAdded = false;
        for (auto sItem = subRoot.begin(); sItem != subRoot.end(); ++sItem)
        {
          if (!strcmp(sItem->c_str(), sub.c_str()))
          {
            alreadyAdded = true;
            break;
          }
        }
        if (!alreadyAdded)
        {
          subRoot.push_back(sub);
        }
      }
    }
  }
  return !subRoot.empty();
}

bool IsRootForAccessibleItems(std::string path)
{
  std::list<std::string> tmp;
  return IsRootForAccessibleItems(PathUWP(path), tmp, true);
}

// Ensure item path without root
std::string resolvePath(std::string& itemName)
{
  // Ensure slashs changed from '/' to '\'
  windowsPath(itemName);

  // Do some fixes because 'TryGetItemAsync' is very sensetive
  replace(itemName, "\\\\", "\\");
  replace(itemName, "//", "/");
  rtrim(itemName, ":"); // remove ':' at the end of the path (if any)

  return itemName;
}

bool checkAccess(std::string path)
{
  bool faccess = false;
  try
  {
    auto tesFolder = winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(convert(path)).get();
    faccess = tesFolder != nullptr;
  }
  catch (...)
  {
  }
  return faccess;
}

int browseFolderCaller = -1;
std::string s_file_selector_browse_directory;
bool dialogRecalled = false;

void BrowseFolder()
{
  winrt::Windows::ApplicationModel::Core::CoreApplication::MainView().CoreWindow().Dispatcher().RunAsync(
    winrt::Windows::UI::Core::CoreDispatcherPriority::Normal, [&]() {
      try
      {
        auto folderPicker{winrt::Windows::Storage::Pickers::FolderPicker()};
        folderPicker.SuggestedStartLocation(winrt::Windows::Storage::Pickers::PickerLocationId::Downloads);
        folderPicker.FileTypeFilter().Append(L"*");

        winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Storage::StorageFolder> folderOperation =
          folderPicker.PickSingleFolderAsync();
        folderOperation.Completed(
          [](winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Storage::StorageFolder> const& operation,
             winrt::Windows::Foundation::AsyncStatus const& status) {
            try
            {
              if (status == winrt::Windows::Foundation::AsyncStatus::Completed)
              {
                winrt::Windows::Storage::StorageFolder selectedFolder = operation.GetResults();
                if (selectedFolder != nullptr)
                {
                  std::wstring folderPath = selectedFolder.Path().c_str();
                  AddItemToFutureList(selectedFolder);
                  s_file_selector_browse_directory = convert(folderPath);
                  dialogRecalled = true;
                  switch (browseFolderCaller)
                  {
                    case 0:
                      FullscreenUI::DoStartFile();
                      break;

                    case 1:
                      FullscreenUI::ChangeDataFolder();
                      break;

                    case 2:
                      FullscreenUI::DoChangeDiscFromFile();
                      break;

                    case 3:
                      // Not ready
                      FullscreenUI::DoSettingInfoButton();
                      break;

                    case 4:
                      FullscreenUI::DoAddSearchDir();
                      break;

                    case 5:
                      FullscreenUI::DoBIOSDirectory();
                      break;

                    case 6:
                      FullscreenUI::DoMemCardDirectory();
                      break;

                    default:
                      FullscreenUI::DoNothing();
                      break;
                  }
                }
              }
              else if (status == winrt::Windows::Foundation::AsyncStatus::Canceled)
              {
              }
              else
              {
              }
            }
            catch (...)
            {
            }
          });
      }
      catch (...)
      {
      }
    });
}

bool IsContainsAccessibleItems(PathUWP path)
{
  path = PathResolver(path);

  for (auto sItem = alist.begin(); sItem != alist.end(); ++sItem)
  {
    std::string sString = std::string(sItem->c_str());
    PathUWP sPath(sString);
    if (isParent(path.ToString(), sString, sPath.GetFilename()))
    {
      return true;
    }
  }

  return false;
}

bool IsContainsAccessibleItems(std::string path)
{
  return IsContainsAccessibleItems(PathUWP(path));
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
    try
    {
      auto dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
      auto dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
      auto dwCreationDisposition = CREATE_ALWAYS;

      auto testFile = std::string(driveName);
      testFile.append("\\.DuckStationAccessCheck");
#if defined(_M_ARM) || defined(_M_ARM64) || defined(BUILD14393)
      HANDLE h = CreateFile2(convertToLPCWSTR(testFile), dwDesiredAccess, dwShareMode, dwCreationDisposition, nullptr);
#else
      // HANDLE h = CreateFile2FromAppW(convertToLPCWSTR(testFile), dwDesiredAccess, dwShareMode, dwCreationDisposition,
      // nullptr);
      HANDLE h = CreateFile2(convertToLPCWSTR(testFile), dwDesiredAccess, dwShareMode, dwCreationDisposition, nullptr);
#endif
      if (h != INVALID_HANDLE_VALUE)
      {
        state = true;
        CloseHandle(h);
#if defined(_M_ARM) || defined(_M_ARM64) || defined(BUILD14393)
        DeleteFileW(convertToLPCWSTR(testFile));
#else
        DeleteFileW(convertToLPCWSTR(testFile));
        // DeleteFileFromAppW(convertToLPCWSTR(testFile));
#endif
      }
      accessState.insert(std::make_pair(driveName, state));
    }
    catch (...)
    {
    }
  }

  if (!state && checkIfContainsFutureAccessItems)
  {
    // Consider the drive accessible in case it contain files/folder selected before to avoid empty results
    state = IsRootForAccessibleItems(driveName) || IsContainsAccessibleItems(driveName);
  }
  return state;
}

time_t filetime_to_timet2(LARGE_INTEGER ull) 
{
  return ull.QuadPart / 10000000ULL - 11644473600ULL;
}

std::string GetInstallationFolder()
{
  return convert(winrt::Windows::ApplicationModel::Package::Current().InstalledLocation().Path());
}
std::string GetTempFolder()
{
  return convert(winrt::Windows::Storage::ApplicationData::Current().TemporaryFolder().Path());
}

template<typename T>
bool findInList(std::list<T> inputList, T str)
{
  return (std::find(inputList.begin(), inputList.end(), str) != inputList.end());
};

std::list<std::string> emptyCache;
void checkEmptyFolder(std::string path)
{
  PathUWP sPath(path);
  auto parent = sPath.GetDirectory();
  std::string dataFolderPath = convert(dataFolder.Path());
  auto finalPath = parent;
  replace(finalPath, (dataFolderPath + "\\"), "");
}

void ResetEmptyList()
{
  emptyCache.clear();
}
bool IsValidUWP(std::string path, bool allowForAppData)
{
  auto p = PathResolver(path);

  // Check valid path
  if (p.Type() == PathTypeUWP::UNDEFINED || !p.IsAbsolute())
  {
    return false;
  }

  bool state = false;

  if (!allowForAppData && !findInList(emptyCache, p.GetDirectory()))
  {
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

bool dataFileExists(std::string path, bool isFolder)
{
  bool exists = false;
  try
  {
    PathUWP sPath(path);
    std::string dataFolderPath = convert(dataFolder.Path());
    auto finalPath = path;

    if (dataFolderPath == path)
    {
      exists = true;
    }
    else
    {
      replace(finalPath, (dataFolderPath + "\\"), "");

      auto testFile = dataFolder.TryGetItemAsync(convert(finalPath)).get();
      if (testFile != nullptr)
      {
        exists = (isFolder || testFile.IsOfType(winrt::Windows::Storage::StorageItemTypes::File));
        testFile = nullptr;
      }
    }
  }
  catch (...)
  {
  }
  return exists;
}

bool dataFileExists(std::string path)
{
  return dataFileExists(path, false);
}

winrt::Windows::Storage::IStorageItem getDataItem(std::string path)
{
  winrt::Windows::Storage::IStorageItem item = nullptr;
  try
  {
    PathUWP sPath(path);
    std::string dataFolderPath = convert(dataFolder.Path());
    auto finalPath = path;
    replace(finalPath, (dataFolderPath + "\\"), "");

    item = dataFolder.TryGetItemAsync(convert(finalPath)).get();
  }
  catch (...)
  {
  }
  return item;
}

bool isDataFile(std::string path)
{
  std::string dataFolderPath = convert(dataFolder.Path());
  return IsValidUWP(path, false) && isChild(dataFolderPath, path);
}

#ifdef _WIN32
static std::time_t ConvertFileTimeToUnixTime(const FILETIME& ft)
{
  // based off https://stackoverflow.com/a/6161842
  static constexpr s64 WINDOWS_TICK = 10000000;
  static constexpr s64 SEC_TO_UNIX_EPOCH = 11644473600LL;

  const s64 full = static_cast<s64>((static_cast<u64>(ft.dwHighDateTime) << 32) | static_cast<u64>(ft.dwLowDateTime));
  return static_cast<std::time_t>(full / WINDOWS_TICK - SEC_TO_UNIX_EPOCH);
}
#endif

static inline bool FileSystemCharacterIsSane(char c, bool StripSlashes)
{
  if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') && !(c >= '0' && c <= '9') && c != ' ' && c != ' ' &&
      c != '_' && c != '-' && c != '.')
  {
    if (!StripSlashes && (c == '/' || c == '\\'))
      return true;

    return false;
  }

  return true;
}

template<typename T>
static inline void PathAppendString(std::string& dst, const T& src)
{
  if (dst.capacity() < (dst.length() + src.length()))
    dst.reserve(dst.length() + src.length());

  bool last_separator = (!dst.empty() && dst.back() == FS_OSPATH_SEPARATOR_CHARACTER);

  size_t index = 0;

#ifdef _WIN32
  // special case for UNC paths here
  if (dst.empty() && src.length() >= 3 && src[0] == '\\' && src[1] == '\\' && src[2] != '\\')
  {
    dst.append("\\\\");
    index = 2;
  }
#endif

  for (; index < src.length(); index++)
  {
    const char ch = src[index];

#ifdef _WIN32
    // convert forward slashes to backslashes
    if (ch == '\\' || ch == '/')
#else
    if (ch == '/')
#endif
    {
      if (last_separator)
        continue;
      last_separator = true;
      dst.push_back(FS_OSPATH_SEPARATOR_CHARACTER);
    }
    else
    {
      last_separator = false;
      dst.push_back(ch);
    }
  }
}

void Path::SanitizeFileName(char* Destination, u32 cbDestination, const char* FileName, bool StripSlashes /* = true */)
{
  u32 i;
  u32 fileNameLength = static_cast<u32>(std::strlen(FileName));

  if (FileName == Destination)
  {
    for (i = 0; i < fileNameLength; i++)
    {
      if (!FileSystemCharacterIsSane(FileName[i], StripSlashes))
        Destination[i] = '_';
    }
  }
  else
  {
    for (i = 0; i < fileNameLength && i < cbDestination; i++)
    {
      if (FileSystemCharacterIsSane(FileName[i], StripSlashes))
        Destination[i] = FileName[i];
      else
        Destination[i] = '_';
    }
  }
}

void Path::SanitizeFileName(std::string& Destination, bool StripSlashes /* = true*/)
{
  const std::size_t len = Destination.length();
  for (std::size_t i = 0; i < len; i++)
  {
    if (!FileSystemCharacterIsSane(Destination[i], StripSlashes))
      Destination[i] = '_';
  }
}

bool Path::IsAbsolute(const std::string_view& path)
{
#ifdef _WIN32
  return (path.length() >= 3 && ((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')) &&
          path[1] == ':' && (path[2] == '/' || path[2] == '\\')) ||
         (path.length() >= 3 && path[0] == '\\' && path[1] == '\\');
#else
  return (path.length() >= 1 && path[0] == '/');
#endif
}

std::string Path::ToNativePath(const std::string_view& path)
{
  std::string ret;
  PathAppendString(ret, path);

  // remove trailing slashes
  if (ret.length() > 1)
  {
    while (ret.back() == FS_OSPATH_SEPARATOR_CHARACTER)
      ret.pop_back();
  }

  return ret;
}

void Path::ToNativePath(std::string* path)
{
  *path = Path::ToNativePath(*path);
}

std::string Path::Canonicalize(const std::string_view& path)
{
  std::vector<std::string_view> components = Path::SplitNativePath(path);
  std::vector<std::string_view> new_components;
  new_components.reserve(components.size());
  for (const std::string_view& component : components)
  {
    if (component == ".")
    {
      // current directory, so it can be skipped, unless it's the only component
      if (components.size() == 1)
        new_components.push_back(std::move(component));
    }
    else if (component == "..")
    {
      // parent directory, pop one off if we're not at the beginning, otherwise preserve.
      if (!new_components.empty())
        new_components.pop_back();
      else
        new_components.push_back(std::move(component));
    }
    else
    {
      // anything else, preserve
      new_components.push_back(std::move(component));
    }
  }

  return Path::JoinNativePath(new_components);
}

void Path::Canonicalize(std::string* path)
{
  *path = Canonicalize(*path);
}

std::string Path::MakeRelative(const std::string_view& path, const std::string_view& relative_to)
{
  // simple algorithm, we just work on the components. could probably be better, but it'll do for now.
  std::vector<std::string_view> path_components(SplitNativePath(path));
  std::vector<std::string_view> relative_components(SplitNativePath(relative_to));
  std::vector<std::string_view> new_components;

  // both must be absolute paths
  if (Path::IsAbsolute(path) && Path::IsAbsolute(relative_to))
  {
    // find the number of same components
    size_t num_same = 0;
    for (size_t i = 0; i < path_components.size() && i < relative_components.size(); i++)
    {
      if (path_components[i] == relative_components[i])
        num_same++;
      else
        break;
    }

    // we need at least one same component
    if (num_same > 0)
    {
      // from the relative_to directory, back up to the start of the common components
      const size_t num_ups = relative_components.size() - num_same;
      for (size_t i = 0; i < num_ups; i++)
        new_components.emplace_back("..");

      // and add the remainder of the path components
      for (size_t i = num_same; i < path_components.size(); i++)
        new_components.push_back(std::move(path_components[i]));
    }
    else
    {
      // no similarity
      new_components = std::move(path_components);
    }
  }
  else
  {
    // not absolute
    new_components = std::move(path_components);
  }

  return JoinNativePath(new_components);
}

std::string_view Path::GetExtension(const std::string_view& path)
{
  const std::string_view::size_type pos = path.rfind('.');
  if (pos == std::string_view::npos)
    return std::string_view();
  else
    return path.substr(pos + 1);
}

std::string_view Path::StripExtension(const std::string_view& path)
{
  const std::string_view::size_type pos = path.rfind('.');
  if (pos == std::string_view::npos)
    return path;

  return path.substr(0, pos);
}

std::string Path::ReplaceExtension(const std::string_view& path, const std::string_view& new_extension)
{
  const std::string_view::size_type pos = path.rfind('.');
  if (pos == std::string_view::npos)
    return std::string(path);

  std::string ret(path, 0, pos + 1);
  ret.append(new_extension);
  return ret;
}

static std::string_view::size_type GetLastSeperatorPosition(const std::string_view& filename, bool include_separator)
{
  std::string_view::size_type last_separator = filename.rfind('/');
  if (include_separator && last_separator != std::string_view::npos)
    last_separator++;

#if defined(_WIN32)
  std::string_view::size_type other_last_separator = filename.rfind('\\');
  if (other_last_separator != std::string_view::npos)
  {
    if (include_separator)
      other_last_separator++;
    if (last_separator == std::string_view::npos || other_last_separator > last_separator)
      last_separator = other_last_separator;
  }
#endif

  return last_separator;
}

std::string FileSystem::GetDisplayNameFromPath(const std::string_view& path)
{
  return std::string(Path::GetFileName(path));
}

std::string_view Path::GetDirectory(const std::string_view& path)
{
  const std::string::size_type pos = GetLastSeperatorPosition(path, false);
  if (pos == std::string_view::npos)
    return {};

  return path.substr(0, pos);
}

std::string_view Path::GetFileName(const std::string_view& path)
{
  const std::string_view::size_type pos = GetLastSeperatorPosition(path, true);
  if (pos == std::string_view::npos)
    return path;

  return path.substr(pos);
}

std::string_view Path::GetFileTitle(const std::string_view& path)
{
  const std::string_view filename(GetFileName(path));
  const std::string::size_type pos = filename.rfind('.');
  if (pos == std::string_view::npos)
    return filename;

  return filename.substr(0, pos);
}

std::string Path::ChangeFileName(const std::string_view& path, const std::string_view& new_filename)
{
  std::string ret;
  PathAppendString(ret, path);

  const std::string_view::size_type pos = GetLastSeperatorPosition(ret, true);
  if (pos == std::string_view::npos)
  {
    ret.clear();
    PathAppendString(ret, new_filename);
  }
  else
  {
    if (!new_filename.empty())
    {
      ret.erase(pos);
      PathAppendString(ret, new_filename);
    }
    else
    {
      ret.erase(pos - 1);
    }
  }

  return ret;
}

void Path::ChangeFileName(std::string* path, const std::string_view& new_filename)
{
  *path = ChangeFileName(*path, new_filename);
}

std::string Path::AppendDirectory(const std::string_view& path, const std::string_view& new_dir)
{
  std::string ret;
  if (!new_dir.empty())
  {
    const std::string_view::size_type pos = GetLastSeperatorPosition(path, true);

    ret.reserve(path.length() + new_dir.length() + 1);
    if (pos != std::string_view::npos)
      PathAppendString(ret, path.substr(0, pos));

    while (!ret.empty() && ret.back() == FS_OSPATH_SEPARATOR_CHARACTER)
      ret.pop_back();

    if (!ret.empty())
      ret += FS_OSPATH_SEPARATOR_CHARACTER;

    PathAppendString(ret, new_dir);

    if (pos != std::string_view::npos)
    {
      const std::string_view filepart(path.substr(pos));
      if (!filepart.empty())
      {
        ret += FS_OSPATH_SEPARATOR_CHARACTER;
        PathAppendString(ret, filepart);
      }
    }
    else if (!path.empty())
    {
      ret += FS_OSPATH_SEPARATOR_CHARACTER;
      PathAppendString(ret, path);
    }
  }
  else
  {
    PathAppendString(ret, path);
  }

  return ret;
}

void Path::AppendDirectory(std::string* path, const std::string_view& new_dir)
{
  *path = AppendDirectory(*path, new_dir);
}

std::vector<std::string_view> Path::SplitWindowsPath(const std::string_view& path)
{
  std::vector<std::string_view> parts;

  std::string::size_type start = 0;
  std::string::size_type pos = 0;

  // preserve unc paths
  if (path.size() > 2 && path[0] == '\\' && path[1] == '\\')
    pos = 2;

  while (pos < path.size())
  {
    if (path[pos] != '/' && path[pos] != '\\')
    {
      pos++;
      continue;
    }

    // skip consecutive separators
    if (pos != start)
      parts.push_back(path.substr(start, pos - start));

    pos++;
    start = pos;
  }

  if (start != pos)
    parts.push_back(path.substr(start));

  return parts;
}

std::string Path::JoinWindowsPath(const std::vector<std::string_view>& components)
{
  return StringUtil::JoinString(components.begin(), components.end(), '\\');
}

std::vector<std::string_view> Path::SplitNativePath(const std::string_view& path)
{
#ifdef _WIN32
  return SplitWindowsPath(path);
#else
  std::vector<std::string_view> parts;

  std::string::size_type start = 0;
  std::string::size_type pos = 0;
  while (pos < path.size())
  {
    if (path[pos] != '/')
    {
      pos++;
      continue;
    }

    // skip consecutive separators
    // for unix, we create an empty element at the beginning when it's an absolute path
    // that way, when it's re-joined later, we preserve the starting slash.
    if (pos != start || pos == 0)
      parts.push_back(path.substr(start, pos - start));

    pos++;
    start = pos;
  }

  if (start != pos)
    parts.push_back(path.substr(start));

  return parts;
#endif
}

std::string Path::JoinNativePath(const std::vector<std::string_view>& components)
{
  return StringUtil::JoinString(components.begin(), components.end(), FS_OSPATH_SEPARATOR_CHARACTER);
}

std::vector<std::string> FileSystem::GetRootDirectoryList()
{
  std::vector<std::string> results;

#if defined(_WIN32) && !defined(_UWP)
  char buf[256];
  const DWORD size = GetLogicalDriveStringsA(sizeof(buf), buf);
  if (size != 0 && size < (sizeof(buf) - 1))
  {
    const char* ptr = buf;
    while (*ptr != '\0')
    {
      const std::size_t len = std::strlen(ptr);
      results.emplace_back(ptr, len);
      ptr += len + 1u;
    }
  }
#elif defined(_UWP)

  /*if (const auto local_location = winrt::Windows::Storage::ApplicationData::Current().LocalFolder(); local_location)
  {
    if (const auto path = local_location.Path(); !path.empty())
      results.push_back(StringUtil::WideStringToUTF8String(path));
  }

  if (const auto local_location = dataFolder; local_location)
  {
    if (const auto path = local_location.Path(); !path.empty())
      results.push_back(StringUtil::WideStringToUTF8String(path));
  }*/

  if (aflist.empty())
  {
    alist.clear();
    GetFutureAccessList();
  }
  for (auto sItem = aflist.begin(); sItem != aflist.end(); ++sItem)
  {
    results.push_back(std::string(sItem->c_str()));
  }

  const auto devices = winrt::Windows::Storage::KnownFolders::RemovableDevices();
  const auto folders_task(devices.GetFoldersAsync());
  for (const auto& storage_folder : folders_task.get())
  {
    const auto path = storage_folder.Path();
    if (!path.empty())
      results.push_back(StringUtil::WideStringToUTF8String(path));
  }
#else
  const char* home_path = std::getenv("HOME");
  if (home_path)
    results.push_back(home_path);

  results.push_back("/");
#endif

  return results;
}

std::string Path::BuildRelativePath(const std::string_view& filename, const std::string_view& new_filename)
{
  std::string new_string;

  std::string_view::size_type pos = GetLastSeperatorPosition(filename, true);
  if (pos != std::string_view::npos)
    new_string.assign(filename, 0, pos);
  new_string.append(new_filename);
  return new_string;
}

std::string Path::Combine(const std::string_view& base, const std::string_view& next)
{
  std::string ret;
  ret.reserve(base.length() + next.length() + 1);

  PathAppendString(ret, base);
  while (!ret.empty() && ret.back() == FS_OSPATH_SEPARATOR_CHARACTER)
    ret.pop_back();

  ret += FS_OSPATH_SEPARATOR_CHARACTER;
  PathAppendString(ret, next);
  while (!ret.empty() && ret.back() == FS_OSPATH_SEPARATOR_CHARACTER)
    ret.pop_back();

  return ret;
}

#ifdef _UWP
static std::FILE* OpenCFileUWP(const wchar_t* wfilename, const wchar_t* mode, FileSystem::FileShareMode share_mode)
{
  DWORD access = 0;
  DWORD share = 0;
  DWORD disposition = 0;

  std::string shareString;

  switch (share_mode)
  {
    case FileSystem::FileShareMode::DenyNone:
      share = FILE_SHARE_READ | FILE_SHARE_WRITE;
      shareString.append("FILE_SHARE_READ | FILE_SHARE_WRITE");
      break;
    case FileSystem::FileShareMode::DenyRead:
      share = FILE_SHARE_WRITE;
      shareString.append("FILE_SHARE_WRITE");
      break;
    case FileSystem::FileShareMode::DenyWrite:
      share = FILE_SHARE_READ;
      shareString.append("FILE_SHARE_READ");
      break;
    case FileSystem::FileShareMode::DenyReadWrite:
    default:
      share = 0;
      shareString.append("0");
      break;
  }

  std::string flagsString;
  std::string accessString;

  int flags = 0;
  const wchar_t* tmode = mode;
  while (*tmode)
  {
    if (*tmode == L'r' && *(tmode + 1) == L'+')
    {
      access = GENERIC_READ | GENERIC_WRITE;
      disposition = OPEN_EXISTING;
      flags |= _O_RDWR;
      tmode += 2;
      flagsString.append("_O_RDWR | ");
      accessString.append("GENERIC_READ | GENERIC_WRITE | ");
    }
    else if (*tmode == L'w' && *(tmode + 1) == L'+')
    {
      access = GENERIC_READ | GENERIC_WRITE;
      disposition = CREATE_ALWAYS;
      flags |= _O_RDWR | _O_CREAT | _O_TRUNC;
      tmode += 2;
      flagsString.append("_O_RDWR | _O_CREAT | _O_TRUNC | ");
      accessString.append("GENERIC_READ | GENERIC_WRITE | ");
    }
    else if (*tmode == L'a' && *(tmode + 1) == L'+')
    {
      access = GENERIC_READ | GENERIC_WRITE;
      disposition = CREATE_ALWAYS;
      flags |= _O_RDWR | _O_APPEND | _O_CREAT | _O_TRUNC;
      tmode += 2;
      flagsString.append("_O_RDWR | _O_APPEND | _O_CREAT | _O_TRUNC | ");
      accessString.append("GENERIC_READ | GENERIC_WRITE | ");
    }
    else if (*tmode == L'r')
    {
      access = GENERIC_READ;
      disposition = OPEN_EXISTING;
      flags |= _O_RDONLY;
      tmode++;
      flagsString.append("_O_RDONLY | ");
      accessString.append("GENERIC_READ | ");
    }
    else if (*tmode == L'w')
    {
      access = GENERIC_WRITE;
      disposition = CREATE_ALWAYS;
      flags |= _O_WRONLY | _O_CREAT | _O_TRUNC;
      tmode++;
      flagsString.append("_O_WRONLY | _O_CREAT | _O_TRUNC | ");
      accessString.append("GENERIC_WRITE | ");
    }
    else if (*tmode == L'a')
    {
      access = GENERIC_READ | GENERIC_WRITE;
      disposition = CREATE_ALWAYS;
      flags |= _O_WRONLY | _O_APPEND | _O_CREAT | _O_TRUNC;
      tmode++;
      flagsString.append("_O_WRONLY | _O_APPEND | _O_CREAT | _O_TRUNC | ");
      accessString.append("GENERIC_READ | GENERIC_WRITE | ");
    }
    else if (*tmode == L'b')
    {
      flags |= _O_BINARY;
      tmode++;
      flagsString.append("_O_BINARY | ");
    }
    else if (*tmode == L'S')
    {
      flags |= _O_SEQUENTIAL;
      tmode++;
      flagsString.append("_O_SEQUENTIAL | ");
    }
    else if (*tmode == L'R')
    {
      flags |= _O_RANDOM;
      tmode++;
      flagsString.append("_O_RANDOM | ");
    }
    else
    {
      Log_ErrorPrintf("Unknown mode flags: '%s'", StringUtil::WideStringToUTF8String(mode).c_str());
      return nullptr;
    }
  }

  HANDLE hFile = CreateFile2Ptr(wfilename, access, share, disposition, nullptr);
  auto fileMode = GetFileMode(convert(std::wstring(mode)).c_str());

  if (hFile == INVALID_HANDLE_VALUE)
  {
    winrt::Windows::Storage::StorageFile testFile = nullptr;
    if (IsValidUWP(convert(std::wstring(wfilename)), false))
    {
      std::string str = convert(std::wstring(wfilename));
      Log_WarningPrintf("Opening file: %s", str.c_str());
      try
      {
        bool createIfNotExists = fileMode->isCreate;
        bool isAppend = fileMode->isAppend;

        try
        {
          if (isDataFile(str))
          {
            if (dataFileExists(str))
            {
              testFile = winrt::Windows::Storage::StorageFile::GetFileFromPathAsync(convert(str)).get();
            }
            else
            {
              if (!createIfNotExists)
              {
                return nullptr;
              }
            }
          }
          else
          {
            testFile = winrt::Windows::Storage::StorageFile::GetFileFromPathAsync(convert(str)).get();
          }
          if (testFile != nullptr)
          {
            if (!fileMode->isAppend && fileMode->isCreate)
            {
              testFile.DeleteAsync().get();
              testFile = nullptr;
            }
            else
            {
              GetFileHandle(testFile, &hFile, GetAccessMode(access), GetShareMode(share));
            }
          }
        }
        catch (...)
        {
        }
        if (testFile == nullptr)
        {
          if (createIfNotExists)
          {
            try
            {
              std::wstring parent =
                StringUtil::UTF8StringToWideString(GetFolderParent(convert(std::wstring(wfilename))));
              std::wstring name = StringUtil::UTF8StringToWideString(GetFolderName(convert(std::wstring(wfilename))));
              auto testFolder =
                winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(winrt::hstring(parent)).get();
              if (testFolder != nullptr)
              {
                auto createTest = testFolder
                                    .CreateFileAsync(winrt::hstring(name),
                                                     winrt::Windows::Storage::CreationCollisionOption::ReplaceExisting)
                                    .get();
                if (createTest != nullptr)
                {
                  GetFileHandle(createTest, &hFile, GetAccessMode(access), GetShareMode(share));
                }
              }
            }
            catch (...)
            {
            }
          }
        }
      }
      catch (...)
      {
      }
    }
  }

  if (hFile == INVALID_HANDLE_VALUE)
  {
    return nullptr;
  }

  if (flags & _O_APPEND && !SetFilePointerEx(hFile, LARGE_INTEGER{}, nullptr, FILE_END))
  {
    Log_ErrorPrintf("SetFilePointerEx() failed: %08X", GetLastError());
    CloseHandle(hFile);
    return nullptr;
  }

  Log_WarningPrintf("Handle ready for access: %s, share: %s", accessString.c_str(), shareString.c_str());
  Log_WarningPrintf("_open_osfhandle() flags: %s", flagsString.c_str());
  int fd = _open_osfhandle(reinterpret_cast<intptr_t>(hFile), flags);
  if (fd < 0)
  {
    CloseHandle(hFile);
    return nullptr;
  }

  Log_WarningPrintf("_wfdopen() mode: %s", convert(std::wstring(mode)).c_str());
  std::FILE* fp = _wfdopen(fd, mode);
  if (!fp)
  {
    _close(fd);
    return nullptr;
  }

  return fp;
}
#endif // _UWP

std::FILE* FileSystem::OpenCFile(const char* filename, const char* mode)
{
#ifdef _WIN32
  const std::wstring wfilename(StringUtil::UTF8StringToWideString(filename));
  const std::wstring wmode(StringUtil::UTF8StringToWideString(mode));
  if (!wfilename.empty() && !wmode.empty())
  {
    std::FILE* fp;
    if (_wfopen_s(&fp, wfilename.c_str(), wmode.c_str()) != 0)
    {
#ifdef _UWP
      return OpenCFileUWP(wfilename.c_str(), wmode.c_str(), FileShareMode::DenyReadWrite);
#else
      return nullptr;
#endif
    }

    return fp;
  }

  std::FILE* fp;
  if (fopen_s(&fp, filename, mode) != 0)
    return nullptr;

  return fp;
#else
  return std::fopen(filename, mode);
#endif
}

int FileSystem::OpenFDFile(const char* filename, int flags, int mode)
{
#ifdef _WIN32
  const std::wstring wfilename(StringUtil::UTF8StringToWideString(filename));
  if (!wfilename.empty())
  {
    // TODO: UWP
    return _wopen(wfilename.c_str(), flags, mode);
  }

  return -1;
#else
  return open(filename, flags, mode);
#endif
}

FileSystem::ManagedCFilePtr FileSystem::OpenManagedCFile(const char* filename, const char* mode)
{
  return ManagedCFilePtr(OpenCFile(filename, mode), [](std::FILE* fp) { std::fclose(fp); });
}

std::FILE* FileSystem::OpenSharedCFile(const char* filename, const char* mode, FileShareMode share_mode)
{
#ifdef _WIN32
  const std::wstring wfilename(StringUtil::UTF8StringToWideString(filename));
  const std::wstring wmode(StringUtil::UTF8StringToWideString(mode));
  if (wfilename.empty() || wmode.empty())
    return nullptr;

  int share_flags = 0;
  switch (share_mode)
  {
    case FileShareMode::DenyNone:
      share_flags = _SH_DENYNO;
      break;
    case FileShareMode::DenyRead:
      share_flags = _SH_DENYRD;
      break;
    case FileShareMode::DenyWrite:
      share_flags = _SH_DENYWR;
      break;
    case FileShareMode::DenyReadWrite:
    default:
      share_flags = _SH_DENYRW;
      break;
  }

  std::FILE* fp = _wfsopen(wfilename.c_str(), wmode.c_str(), share_flags);
  if (fp)
    return fp;

#ifdef _UWP
  return OpenCFileUWP(wfilename.c_str(), wmode.c_str(), share_mode);
#else
  return nullptr;
#endif
#else
  return std::fopen(filename, mode);
#endif
}

FileSystem::ManagedCFilePtr FileSystem::OpenManagedSharedCFile(const char* filename, const char* mode,
                                                               FileShareMode share_mode)
{
  return ManagedCFilePtr(OpenSharedCFile(filename, mode, share_mode), [](std::FILE* fp) { std::fclose(fp); });
}

int FileSystem::FSeek64(std::FILE* fp, s64 offset, int whence)
{
#ifdef _WIN32
  return _fseeki64(fp, offset, whence);
#else
  // Prevent truncation on platforms which don't have a 64-bit off_t.
  if constexpr (sizeof(off_t) != sizeof(s64))
  {
    if (offset < std::numeric_limits<off_t>::min() || offset > std::numeric_limits<off_t>::max())
      return -1;
  }

  return fseeko(fp, static_cast<off_t>(offset), whence);
#endif
}

s64 FileSystem::FTell64(std::FILE* fp)
{
#ifdef _WIN32
  return static_cast<s64>(_ftelli64(fp));
#else
  return static_cast<s64>(ftello(fp));
#endif
}

s64 FileSystem::FSize64(std::FILE* fp)
{
  const s64 pos = FTell64(fp);
  if (pos >= 0)
  {
    if (FSeek64(fp, 0, SEEK_END) == 0)
    {
      const s64 size = FTell64(fp);
      if (FSeek64(fp, pos, SEEK_SET) == 0)
        return size;
    }
  }

  return -1;
}

s64 FileSystem::GetPathFileSize(const char* Path)
{
  FILESYSTEM_STAT_DATA sd;
  if (!StatFile(Path, &sd))
    return -1;

  return sd.Size;
}

std::optional<std::vector<u8>> FileSystem::ReadBinaryFile(const char* filename)
{
  ManagedCFilePtr fp = OpenManagedCFile(filename, "rb");
  if (!fp)
    return std::nullopt;

  return ReadBinaryFile(fp.get());
}

std::optional<std::vector<u8>> FileSystem::ReadBinaryFile(std::FILE* fp)
{
  std::fseek(fp, 0, SEEK_END);
  const long size = std::ftell(fp);
  std::fseek(fp, 0, SEEK_SET);
  if (size < 0)
    return std::nullopt;

  std::vector<u8> res(static_cast<size_t>(size));
  if (size > 0 && std::fread(res.data(), 1u, static_cast<size_t>(size), fp) != static_cast<size_t>(size))
    return std::nullopt;

  return res;
}

std::optional<std::string> FileSystem::ReadFileToString(const char* filename)
{
  ManagedCFilePtr fp = OpenManagedCFile(filename, "rb");
  if (!fp)
    return std::nullopt;

  return ReadFileToString(fp.get());
}

std::optional<std::string> FileSystem::ReadFileToString(std::FILE* fp)
{
  std::fseek(fp, 0, SEEK_END);
  const long size = std::ftell(fp);
  std::fseek(fp, 0, SEEK_SET);
  if (size < 0)
    return std::nullopt;

  std::string res;
  res.resize(static_cast<size_t>(size));
  // NOTE - assumes mode 'rb', for example, this will fail over missing Windows carriage return bytes
  if (size > 0 && std::fread(res.data(), 1u, static_cast<size_t>(size), fp) != static_cast<size_t>(size))
    return std::nullopt;

  return res;
}

bool FileSystem::WriteBinaryFile(const char* filename, const void* data, size_t data_length)
{
  ManagedCFilePtr fp = OpenManagedCFile(filename, "wb");
  if (!fp)
    return false;

  if (data_length > 0 && std::fwrite(data, 1u, data_length, fp.get()) != data_length)
    return false;

  return true;
}

bool FileSystem::WriteStringToFile(const char* filename, const std::string_view& sv)
{
  ManagedCFilePtr fp = OpenManagedCFile(filename, "wb");
  if (!fp)
    return false;

  if (sv.length() > 0 && std::fwrite(sv.data(), 1u, sv.length(), fp.get()) != sv.length())
    return false;

  return true;
}

bool FileSystem::EnsureDirectoryExists(const char* path, bool recursive)
{
  if (FileSystem::DirectoryExists(path))
    return true;

  // if it fails to create, we're not going to be able to use it anyway
  return FileSystem::CreateDirectory(path, recursive);
}

bool FileSystem::RecursiveDeleteDirectory(const char* path)
{
  FindResultsArray results;
  if (FindFiles(path, "*", FILESYSTEM_FIND_FILES | FILESYSTEM_FIND_FOLDERS | FILESYSTEM_FIND_HIDDEN_FILES, &results))
  {
    for (const FILESYSTEM_FIND_DATA& fd : results)
    {
      if (fd.Attributes & FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY)
      {
        if (!RecursiveDeleteDirectory(fd.FileName.c_str()))
          return false;
      }
      else
      {
        if (!DeleteFile(fd.FileName.c_str()))
          return false;
      }
    }
  }

  return DeleteDirectory(path);
}

bool FileSystem::CopyFilePath(const char* source, const char* destination, bool replace)
{
#ifndef _WIN32
  // TODO: There's technically a race here between checking and opening the file..
  // But fopen doesn't specify any way to say "don't create if it exists"...
  if (!replace && FileExists(destination))
    return false;

  auto in_fp = OpenManagedCFile(source, "rb");
  if (!in_fp)
    return false;

  auto out_fp = OpenManagedCFile(destination, "wb");
  if (!out_fp)
    return false;

  u8 buf[4096];
  while (!std::feof(in_fp.get()))
  {
    size_t bytes_in = std::fread(buf, 1, sizeof(buf), in_fp.get());
    if ((bytes_in == 0 && !std::feof(in_fp.get())) ||
        (bytes_in > 0 && std::fwrite(buf, 1, bytes_in, out_fp.get()) != bytes_in))
    {
      out_fp.reset();
      DeleteFile(destination);
      return false;
    }
  }

  if (std::fflush(out_fp.get()) != 0)
  {
    out_fp.reset();
    DeleteFile(destination);
    return false;
  }

  return true;
#else
  bool isOk = false;
  isOk = CopyFileWPtr(StringUtil::UTF8StringToWideString(source).c_str(),
                      StringUtil::UTF8StringToWideString(destination).c_str(), !replace);
  if (!isOk)
  {
    if (IsValidUWP(std::string(source), true) && IsValidUWP(std::string(destination), true))
    {
      try
      {
        std::string str(source);
        resolvePath(str);
        winrt::Windows::Storage::StorageFile testFile = nullptr;
        if (isDataFile(str))
        {
          if (dataFileExists(str))
          {
            testFile = winrt::Windows::Storage::StorageFile::GetFileFromPathAsync(convert(str)).get();
          }
        }
        else
        {
          testFile = winrt::Windows::Storage::StorageFile::GetFileFromPathAsync(convert(str)).get();
        }

        if (testFile != nullptr)
        {
          auto parent = GetFolderParent(std::string(destination));
          auto name = GetFolderName(std::string(destination));
          auto testFolder =
            (winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(convert(std::string(parent))).get());
          if (testFolder != nullptr)
          {
            if (replace)
            {
              auto copiedFile =
                testFile
                  .CopyAsync(testFolder, convert(name), winrt::Windows::Storage::NameCollisionOption::ReplaceExisting)
                  .get();
              if (copiedFile != nullptr)
              {
                isOk = true;
              }
            }
            else
            {
              auto testExists = testFolder.TryGetItemAsync(convert(std::string(name))).get();
              if (testExists == nullptr)
              {
                auto copiedFile = testFile.CopyAsync(testFolder, convert(name)).get();
                if (copiedFile != nullptr)
                {
                  isOk = true;
                }
              }
              else
              {
                isOk = true;
              }
            }
          }
        }
      }
      catch (...)
      {
      }
    }
  }
  return isOk;
#endif
}

#ifdef _WIN32

static u32 TranslateWin32Attributes(u32 Win32Attributes)
{
  u32 r = 0;

  if (Win32Attributes & FILE_ATTRIBUTE_DIRECTORY)
    r |= FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY;
  if (Win32Attributes & FILE_ATTRIBUTE_READONLY)
    r |= FILESYSTEM_FILE_ATTRIBUTE_READ_ONLY;
  if (Win32Attributes & FILE_ATTRIBUTE_COMPRESSED)
    r |= FILESYSTEM_FILE_ATTRIBUTE_COMPRESSED;

  return r;
}

static DWORD WrapGetFileAttributes(const wchar_t* path)
{
#ifndef _UWP
  return GetFileAttributesW(path);
#else
  DWORD result = INVALID_FILE_ATTRIBUTES;
  WIN32_FILE_ATTRIBUTE_DATA fad;
  if (GetFileAttributesExWPtr(path, GetFileExInfoStandard, &fad))
  {
    result = fad.dwFileAttributes;
  }
  if (result == INVALID_FILE_ATTRIBUTES)
  {
    if (IsValidUWP(convert(std::wstring(path)), false))
    {
      try
      {
        winrt::Windows::Storage::IStorageItem testItem = nullptr;
        std::string str = convert(std::wstring(path));
        resolvePath(str);
        try
        {
          if (isDataFile(str))
          {
            if (dataFileExists(str))
            {
              testItem =
                (winrt::Windows::Storage::IStorageItem)winrt::Windows::Storage::StorageFile::GetFileFromPathAsync(
                  convert(str))
                  .get();
            }
          }
          else
          {
            testItem =
              (winrt::Windows::Storage::IStorageItem)winrt::Windows::Storage::StorageFile::GetFileFromPathAsync(
                convert(str))
                .get();
          }
        }
        catch (...)
        {
        }
        try
        {
          if (testItem == nullptr)
          {
            if (isDataFile(str))
            {
              if (dataFileExists(str, true))
              {
                testItem =
                  (winrt::Windows::Storage::IStorageItem)winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(
                    winrt::hstring(convert(str)))
                    .get();
              }
            }
            else
            {
              testItem =
                (winrt::Windows::Storage::IStorageItem)winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(
                  winrt::hstring(convert(str)))
                  .get();
            }
          }
        }
        catch (...)
        {
        }
        if (testItem != nullptr)
        {
          result = (uint64_t)testItem.Attributes();
          // HANDLE handle;
          // HRESULT hr = GetFileHandle(testFile, &handle, GetAccessMode(GENERIC_READ), GetShareMode(FILE_SHARE_READ));

          // size_t size = sizeof(FILE_BASIC_INFO);
          // FILE_BASIC_INFO* information = (FILE_BASIC_INFO*)(malloc(size));
          // if (hr == S_OK && handle != INVALID_HANDLE_VALUE && information)
          //{
          //   information->FileAttributes = (uint64_t)testFile.Attributes();

          //  if (FALSE == GetFileInformationByHandleEx(handle, FileBasicInfo, information, (DWORD)size))
          //  {
          //    // Fallback to UWP method (Slow)
          //    auto props = testFile.GetBasicPropertiesAsync().get();
          //    information->ChangeTime.QuadPart = winrt::clock::to_file_time(props.DateModified()).value;
          //    information->CreationTime.QuadPart = winrt::clock::to_file_time(props.ItemDate()).value;
          //    information->LastAccessTime.QuadPart = winrt::clock::to_file_time(props.DateModified()).value;
          //    information->LastWriteTime.QuadPart = winrt::clock::to_file_time(props.DateModified()).value;
          //  }
          //  CloseHandle(handle);

          //  result = information->FileAttributes;
          //}
        }
      }
      catch (...)
      {
      }
    }
  }

  return result;
#endif
}

static u32 RecursiveFindFiles(const char* origin_path, const char* parent_path, const char* path, const char* pattern,
                              u32 flags, FileSystem::FindResultsArray* results)
{
  std::string tempStr;
  std::string tempStr2;
  if (path)
  {
    if (parent_path)
    {
      tempStr = StringUtil::StdStringFromFormat("%s\\%s\\%s\\*", origin_path, parent_path, path);
      tempStr2 = StringUtil::StdStringFromFormat("%s\\%s\\%s", origin_path, parent_path, path);
    }
    else
    {
      tempStr = StringUtil::StdStringFromFormat("%s\\%s\\*", origin_path, path);
      tempStr2 = StringUtil::StdStringFromFormat("%s\\%s", origin_path, path);
    }
  }
  else
  {
    tempStr = StringUtil::StdStringFromFormat("%s\\*", origin_path);
    tempStr2 = StringUtil::StdStringFromFormat("%s", origin_path);
  }

  // holder for utf-8 conversion
  WIN32_FIND_DATAW wfd;
  std::string utf8_filename;
  utf8_filename.reserve((sizeof(wfd.cFileName) / sizeof(wfd.cFileName[0])) * 2);

#ifndef _UWP
  HANDLE hFind = FindFirstFileW(StringUtil::UTF8StringToWideString(tempStr).c_str(), &wfd);
#else
  HANDLE hFind = FindFirstFileExWPtr(StringUtil::UTF8StringToWideString(tempStr).c_str(), FindExInfoBasic, &wfd,
                                     FindExSearchNameMatch, nullptr, 0);
#endif

  if (hFind == INVALID_HANDLE_VALUE)
  {
    u32 filesCount = 0;
    bool valid = false;
    if (IsValidUWP(std::string(tempStr2), false))
    {
      try
      {
        winrt::Windows::Storage::StorageFolder testItem = nullptr;
        try
        {
          if (isDataFile(tempStr2))
          {
            if (dataFileExists(tempStr2, true))
            {
              testItem = winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(convert(tempStr2)).get();
            }
          }
          else
          {
            testItem = winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(convert(tempStr2)).get();
          }
        }
        catch (...)
        {
        }
        if (testItem != nullptr)
        {
          valid = true;
          auto folderFiles = testItem.GetItemsAsync().get();
          if (folderFiles != nullptr && folderFiles.Size() > 0)
          {
            bool hasWildCards = false;
            bool wildCardMatchAll = false;
            u32 nFiles = 0;
            if (std::strpbrk(pattern, "*?") != nullptr)
            {
              hasWildCards = true;
              wildCardMatchAll = !(std::strcmp(pattern, "*"));
            }
            for (int i = 0; i < folderFiles.Size(); i++)
            {
              auto item = folderFiles.GetAt(i);
              if (!StringUtil::WideStringToUTF8String(utf8_filename, item.Name().c_str()))
                continue;

              FILESYSTEM_FIND_DATA outData;
              outData.Attributes = 0;
              auto attrs = (uint64_t)item.Attributes();
              if (attrs & FILE_ATTRIBUTE_DIRECTORY)
              {
                if (flags & FILESYSTEM_FIND_RECURSIVE)
                {
                  // recurse into this directory
                  if (parent_path != nullptr)
                  {
                    const std::string recurseDir = StringUtil::StdStringFromFormat("%s\\%s", parent_path, path);
                    filesCount += RecursiveFindFiles(origin_path, recurseDir.c_str(), utf8_filename.c_str(), pattern,
                                                     flags, results);
                  }
                  else
                  {
                    filesCount += RecursiveFindFiles(origin_path, path, utf8_filename.c_str(), pattern, flags, results);
                  }
                }
                if (!(flags & FILESYSTEM_FIND_FOLDERS))
                  continue;

                outData.Attributes |= FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY;
              }
              else
              {
                if (!(flags & FILESYSTEM_FIND_FILES))
                  continue;
              }

              if (attrs & FILE_ATTRIBUTE_READONLY)
                outData.Attributes |= FILESYSTEM_FILE_ATTRIBUTE_READ_ONLY;

              // match the filename
              if (hasWildCards)
              {
                if (!wildCardMatchAll && !StringUtil::WildcardMatch(utf8_filename.c_str(), pattern))
                  continue;
              }
              else
              {
                if (std::strcmp(utf8_filename.c_str(), pattern) != 0)
                  continue;
              }

              // add file to list
              // TODO string formatter, clean this mess..
              if (!(flags & FILESYSTEM_FIND_RELATIVE_PATHS))
              {
                if (parent_path != nullptr)
                  outData.FileName = StringUtil::StdStringFromFormat("%s\\%s\\%s\\%s", origin_path, parent_path, path,
                                                                     utf8_filename.c_str());
                else if (path != nullptr)
                  outData.FileName =
                    StringUtil::StdStringFromFormat("%s\\%s\\%s", origin_path, path, utf8_filename.c_str());
                else
                  outData.FileName = StringUtil::StdStringFromFormat("%s\\%s", origin_path, utf8_filename.c_str());
              }
              else
              {
                if (parent_path != nullptr)
                  outData.FileName =
                    StringUtil::StdStringFromFormat("%s\\%s\\%s", parent_path, path, utf8_filename.c_str());
                else if (path != nullptr)
                  outData.FileName = StringUtil::StdStringFromFormat("%s\\%s", path, utf8_filename.c_str());
                else
                  outData.FileName = utf8_filename;
              }

              outData.CreationTime = 1;
              outData.ModificationTime = 1;

              auto props = item.GetBasicPropertiesAsync().get();
              outData.Size = static_cast<u64>(props.Size());

              size_t size = sizeof(FILE_BASIC_INFO);
              FILE_BASIC_INFO* information = (FILE_BASIC_INFO*)(malloc(size));
              if (information)
              {
                try
                {
                  auto itemDate = winrt::clock::to_file_time(props.ItemDate()).value;
                  auto itemModify = winrt::clock::to_file_time(props.DateModified()).value;

                  information->ChangeTime.QuadPart = winrt::clock::to_file_time(props.DateModified()).value;
                  information->CreationTime.QuadPart = winrt::clock::to_file_time(props.ItemDate()).value;
                  information->LastAccessTime.QuadPart = winrt::clock::to_file_time(props.DateModified()).value;
                  information->LastWriteTime.QuadPart = winrt::clock::to_file_time(props.DateModified()).value;

                  outData.CreationTime = (uint64_t)filetime_to_timet2(information->CreationTime);
                  outData.ModificationTime = (uint64_t)filetime_to_timet2(information->ChangeTime);
                }
                catch (...)
                {
                }
              }

              filesCount++;
              results->push_back(std::move(outData));
            }
          }
        }
      }
      catch (...)
      {
      }
    }

    if (!valid)
    {
      if ((flags & FILESYSTEM_FIND_FOLDERS))
      {
        std::list<std::string> subRoot;
        if (IsRootForAccessibleItems(PathUWP(tempStr2), subRoot))
        {
          if (!subRoot.empty())
          {
            for (auto sub = subRoot.begin(); sub != subRoot.end(); ++sub)
            {
              FILESYSTEM_FIND_DATA outData;
              outData.Attributes |= FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY;
              outData.CreationTime = 1;
              outData.ModificationTime = 1;
              outData.Size = 1;
              PathUWP subPath(std::string(sub->c_str()));
              utf8_filename = subPath.GetFilename();

              if (!(flags & FILESYSTEM_FIND_RELATIVE_PATHS))
              {
                if (parent_path != nullptr)
                  outData.FileName = StringUtil::StdStringFromFormat("%s\\%s\\%s\\%s", origin_path, parent_path, path,
                                                                     utf8_filename.c_str());
                else if (path != nullptr)
                  outData.FileName =
                    StringUtil::StdStringFromFormat("%s\\%s\\%s", origin_path, path, utf8_filename.c_str());
                else
                  outData.FileName = StringUtil::StdStringFromFormat("%s\\%s", origin_path, utf8_filename.c_str());
              }
              else
              {
                if (parent_path != nullptr)
                  outData.FileName =
                    StringUtil::StdStringFromFormat("%s\\%s\\%s", parent_path, path, utf8_filename.c_str());
                else if (path != nullptr)
                  outData.FileName = StringUtil::StdStringFromFormat("%s\\%s", path, utf8_filename.c_str());
                else
                  outData.FileName = utf8_filename;
              }

              results->push_back(std::move(outData));
              filesCount++;
            }
          }
        }
      }
    }
    return filesCount;
  }

  // small speed optimization for '*' case
  bool hasWildCards = false;
  bool wildCardMatchAll = false;
  u32 nFiles = 0;
  if (std::strpbrk(pattern, "*?") != nullptr)
  {
    hasWildCards = true;
    wildCardMatchAll = !(std::strcmp(pattern, "*"));
  }

  // iterate results
  do
  {
    if (wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN && !(flags & FILESYSTEM_FIND_HIDDEN_FILES))
      continue;

    if (wfd.cFileName[0] == L'.')
    {
      if (wfd.cFileName[1] == L'\0' || (wfd.cFileName[1] == L'.' && wfd.cFileName[2] == L'\0'))
        continue;
    }

    if (!StringUtil::WideStringToUTF8String(utf8_filename, wfd.cFileName))
      continue;

    FILESYSTEM_FIND_DATA outData;
    outData.Attributes = 0;

    if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
      if (flags & FILESYSTEM_FIND_RECURSIVE)
      {
        // recurse into this directory
        if (parent_path != nullptr)
        {
          const std::string recurseDir = StringUtil::StdStringFromFormat("%s\\%s", parent_path, path);
          nFiles += RecursiveFindFiles(origin_path, recurseDir.c_str(), utf8_filename.c_str(), pattern, flags, results);
        }
        else
        {
          nFiles += RecursiveFindFiles(origin_path, path, utf8_filename.c_str(), pattern, flags, results);
        }
      }

      if (!(flags & FILESYSTEM_FIND_FOLDERS))
        continue;

      outData.Attributes |= FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY;
    }
    else
    {
      if (!(flags & FILESYSTEM_FIND_FILES))
        continue;
    }

    if (wfd.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
      outData.Attributes |= FILESYSTEM_FILE_ATTRIBUTE_READ_ONLY;

    // match the filename
    if (hasWildCards)
    {
      if (!wildCardMatchAll && !StringUtil::WildcardMatch(utf8_filename.c_str(), pattern))
        continue;
    }
    else
    {
      if (std::strcmp(utf8_filename.c_str(), pattern) != 0)
        continue;
    }

    // add file to list
    // TODO string formatter, clean this mess..
    if (!(flags & FILESYSTEM_FIND_RELATIVE_PATHS))
    {
      if (parent_path != nullptr)
        outData.FileName =
          StringUtil::StdStringFromFormat("%s\\%s\\%s\\%s", origin_path, parent_path, path, utf8_filename.c_str());
      else if (path != nullptr)
        outData.FileName = StringUtil::StdStringFromFormat("%s\\%s\\%s", origin_path, path, utf8_filename.c_str());
      else
        outData.FileName = StringUtil::StdStringFromFormat("%s\\%s", origin_path, utf8_filename.c_str());
    }
    else
    {
      if (parent_path != nullptr)
        outData.FileName = StringUtil::StdStringFromFormat("%s\\%s\\%s", parent_path, path, utf8_filename.c_str());
      else if (path != nullptr)
        outData.FileName = StringUtil::StdStringFromFormat("%s\\%s", path, utf8_filename.c_str());
      else
        outData.FileName = utf8_filename;
    }

    outData.CreationTime = ConvertFileTimeToUnixTime(wfd.ftCreationTime);
    outData.ModificationTime = ConvertFileTimeToUnixTime(wfd.ftLastWriteTime);
    outData.Size = (static_cast<u64>(wfd.nFileSizeHigh) << 32) | static_cast<u64>(wfd.nFileSizeLow);

    nFiles++;
    results->push_back(std::move(outData));
  } while (FindNextFileWPtr(hFind, &wfd) == TRUE);
  FindClose(hFind);

  return nFiles;
}

bool FileSystem::FindFiles(const char* path, const char* pattern, u32 flags, FindResultsArray* results)
{
  // has a path
  if (path[0] == '\0')
    return false;

  // clear result array
  if (!(flags & FILESYSTEM_FIND_KEEP_ARRAY))
    results->clear();

  // enter the recursive function
  return (RecursiveFindFiles(path, nullptr, nullptr, pattern, flags, results) > 0);
}

static void TranslateStat64(struct stat* st, const struct _stat64& st64)
{
  static constexpr __int64 MAX_SIZE = static_cast<__int64>(std::numeric_limits<decltype(st->st_size)>::max());
  st->st_dev = st64.st_dev;
  st->st_ino = st64.st_ino;
  st->st_mode = st64.st_mode;
  st->st_nlink = st64.st_nlink;
  st->st_uid = st64.st_uid;
  st->st_rdev = st64.st_rdev;
  st->st_size = static_cast<decltype(st->st_size)>((st64.st_size > MAX_SIZE) ? MAX_SIZE : st64.st_size);
  st->st_atime = static_cast<time_t>(st64.st_atime);
  st->st_mtime = static_cast<time_t>(st64.st_mtime);
  st->st_ctime = static_cast<time_t>(st64.st_ctime);
}

bool FileSystem::StatFile(const char* path, struct stat* st)
{
  // has a path
  if (path[0] == '\0')
    return false;

  // convert to wide string
  const std::wstring wpath(StringUtil::UTF8StringToWideString(path));
  if (wpath.empty())
    return false;

  struct _stat64 st64;
  if (_wstati64(wpath.c_str(), &st64) != 0)
    return false;

  TranslateStat64(st, st64);
  return true;
}

bool FileSystem::StatFile(std::FILE* fp, struct stat* st)
{
  const int fd = _fileno(fp);
  if (fd < 0)
    return false;

  struct _stat64 st64;
  if (_fstati64(fd, &st64) != 0)
    return false;

  TranslateStat64(st, st64);
  return true;
}

bool FileSystem::StatFile(const char* path, FILESYSTEM_STAT_DATA* sd)
{
  // has a path
  if (path[0] == '\0')
    return false;

  // convert to wide string
  const std::wstring wpath(StringUtil::UTF8StringToWideString(path));
  if (wpath.empty())
    return false;

#ifndef _UWP
  // determine attributes for the path. if it's a directory, things have to be handled differently..
  DWORD fileAttributes = GetFileAttributesW(wpath.c_str());
  if (fileAttributes == INVALID_FILE_ATTRIBUTES)
    return false;

  // test if it is a directory
  HANDLE hFile;
  if (fileAttributes & FILE_ATTRIBUTE_DIRECTORY)
  {
    hFile = CreateFileW(wpath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
  }
  else
  {
    hFile = CreateFileW(wpath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                        OPEN_EXISTING, 0, nullptr);
  }

  // createfile succeded?
  if (hFile == INVALID_HANDLE_VALUE)
    return false;

  // use GetFileInformationByHandle
  BY_HANDLE_FILE_INFORMATION bhfi;
  if (GetFileInformationByHandle(hFile, &bhfi) == FALSE)
  {
    CloseHandle(hFile);
    return false;
  }

  // close handle
  CloseHandle(hFile);

  // fill in the stat data
  sd->Attributes = TranslateWin32Attributes(bhfi.dwFileAttributes);
  sd->CreationTime = ConvertFileTimeToUnixTime(bhfi.ftCreationTime);
  sd->ModificationTime = ConvertFileTimeToUnixTime(bhfi.ftLastWriteTime);
  sd->Size = static_cast<s64>(((u64)bhfi.nFileSizeHigh) << 32 | (u64)bhfi.nFileSizeLow);
  return true;
#else
  bool isOk = false;
  WIN32_FILE_ATTRIBUTE_DATA fad;
  if (GetFileAttributesExWPtr(wpath.c_str(), GetFileExInfoStandard, &fad))
  {
    isOk = true;
    sd->Attributes = TranslateWin32Attributes(fad.dwFileAttributes);
    sd->CreationTime = ConvertFileTimeToUnixTime(fad.ftCreationTime);
    sd->ModificationTime = ConvertFileTimeToUnixTime(fad.ftLastWriteTime);
    sd->Size = static_cast<s64>(((u64)fad.nFileSizeHigh) << 32 | (u64)fad.nFileSizeLow);
  }
  if (!isOk)
  {
    if (IsValidUWP(convert(wpath), false))
    {
      try
      {
        winrt::Windows::Storage::IStorageItem testItem = nullptr;
        std::string str = convert(wpath);
        resolvePath(str);
        try
        {
          if (isDataFile(str))
          {
            if (dataFileExists(str))
            {
              testItem =
                (winrt::Windows::Storage::IStorageItem)winrt::Windows::Storage::StorageFile::GetFileFromPathAsync(
                  convert(str))
                  .get();
            }
          }
          else
          {
            testItem =
              (winrt::Windows::Storage::IStorageItem)winrt::Windows::Storage::StorageFile::GetFileFromPathAsync(
                convert(str))
                .get();
          }
        }
        catch (...)
        {
        }
        try
        {
          if (testItem == nullptr)
          {
            if (isDataFile(str))
            {
              if (dataFileExists(str, true))
              {
                testItem =
                  (winrt::Windows::Storage::IStorageItem)winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(
                    winrt::hstring(convert(str)))
                    .get();
              }
            }
            else
            {
              testItem =
                (winrt::Windows::Storage::IStorageItem)winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(
                  winrt::hstring(convert(str)))
                  .get();
            }
          }
        }
        catch (...)
        {
        }
        if (testItem != nullptr)
        {
          sd->Attributes = TranslateWin32Attributes((uint64_t)testItem.Attributes());
          sd->CreationTime = 1;
          sd->ModificationTime = 1;

          auto props = testItem.GetBasicPropertiesAsync().get();
          sd->Size = static_cast<s64>(props.Size());

          isOk = true;
        }
      }
      catch (...)
      {
      }
    }
  }
  return isOk;
#endif
}

bool FileSystem::StatFile(std::FILE* fp, FILESYSTEM_STAT_DATA* sd)
{
  const int fd = _fileno(fp);
  if (fd < 0)
    return false;

  struct _stat64 st;
  if (_fstati64(fd, &st) != 0)
    return false;

  // parse attributes
  sd->CreationTime = st.st_ctime;
  sd->ModificationTime = st.st_mtime;
  sd->Attributes = 0;
  if ((st.st_mode & _S_IFMT) == _S_IFDIR)
    sd->Attributes |= FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY;

  // parse size
  if ((st.st_mode & _S_IFMT) == _S_IFREG)
    sd->Size = st.st_size;
  else
    sd->Size = 0;

  return true;
}

bool FileSystem::FileExists(const char* path)
{
  // has a path
  if (path[0] == '\0')
    return false;

  // convert to wide string
  const std::wstring wpath(StringUtil::UTF8StringToWideString(path));
  if (wpath.empty())
    return false;

  std::string sPath = convert(wpath);
  if (isDataFile(sPath))
  {
    return dataFileExists(sPath);
  }
  // determine attributes for the path. if it's a directory, things have to be handled differently..
  DWORD fileAttributes = WrapGetFileAttributes(wpath.c_str());
  if (fileAttributes == INVALID_FILE_ATTRIBUTES)
    return false;

  if (fileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    return false;
  else
    return true;
}

bool FileSystem::DirectoryExists(const char* path)
{
  // has a path
  if (path[0] == '\0')
    return false;

  // convert to wide string
  const std::wstring wpath(StringUtil::UTF8StringToWideString(path));
  if (wpath.empty())
    return false;

  // determine attributes for the path. if it's a directory, things have to be handled differently..
  DWORD fileAttributes = WrapGetFileAttributes(wpath.c_str());
  if (fileAttributes == INVALID_FILE_ATTRIBUTES)
    return false;

  if (fileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    return true;
  else
    return false;
}

bool FileSystem::DirectoryIsEmpty(const char* path)
{
  std::wstring wpath(StringUtil::UTF8StringToWideString(path));
  wpath += L"\\*";

  WIN32_FIND_DATAW wfd;
#ifndef _UWP
  HANDLE hFind = FindFirstFileW(wpath.c_str(), &wfd);
#else
  HANDLE hFind = FindFirstFileExWPtr(wpath.c_str(), FindExInfoBasic, &wfd, FindExSearchNameMatch, nullptr, 0);
#endif

  if (hFind == INVALID_HANDLE_VALUE)
  {
    bool isEmpty = true;
    std::string str(convert(wpath));
    if (IsValidUWP(str, false))
    {
      try
      {
        winrt::Windows::Storage::StorageFolder testItem = nullptr;
        if (isDataFile(str))
        {
          if (dataFileExists(str, true))
          {
            testItem = winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(convert(str)).get();
          }
        }
        else
        {
          testItem = winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(convert(str)).get();
        }
        if (testItem != nullptr)
        {
          auto folderFiles = testItem.GetItemsAsync().get();
          if (folderFiles != nullptr)
          {
            isEmpty = folderFiles.Size() == 0;
          }
        }
      }
      catch (...)
      {
      }
    }
    return isEmpty;
  }

  do
  {
    if (wfd.cFileName[0] == L'.')
    {
      if (wfd.cFileName[1] == L'\0' || (wfd.cFileName[1] == L'.' && wfd.cFileName[2] == L'\0'))
        continue;
    }

    FindClose(hFind);
    return false;
  } while (FindNextFileWPtr(hFind, &wfd));

  FindClose(hFind);
  return true;
}

bool FileSystem::CreateDirectory(const char* Path, bool Recursive)
{
  const std::wstring wpath(StringUtil::UTF8StringToWideString(Path));

  // has a path
  if (wpath.empty())
    return false;

    // try just flat-out, might work if there's no other segments that have to be made
#ifndef _UWP
  if (CreateDirectoryW(wpath.c_str(), nullptr))
    return true;
#else
  if (CreateDirectoryWPtr(wpath.c_str(), nullptr))
    return true;
#endif
  std::string str(convert(wpath));
  if (IsValidUWP(str, false))
  {
    try
    {
      winrt::Windows::Storage::StorageFolder createTest = nullptr;

      if (isDataFile(str))
      {
        if (dataFileExists(str, true))
        {
          createTest = winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(convert(str)).get();
        }
      }
      else
      {
        createTest = winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(convert(str)).get();
      }

      if (createTest != nullptr)
      {
        return true;
      }
    }
    catch (...)
    {
    }

    try
    {
      std::wstring parent = StringUtil::UTF8StringToWideString(GetFolderParent(std::string(Path)));
      std::wstring name = StringUtil::UTF8StringToWideString(GetFolderName(std::string(Path)));
      auto testFolder = winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(winrt::hstring(parent)).get();
      if (testFolder != nullptr)
      {
        auto createTest =
          testFolder
            .CreateFolderAsync(winrt::hstring(name), winrt::Windows::Storage::CreationCollisionOption::OpenIfExists)
            .get();
        if (createTest != nullptr)
        {
          return true;
        }
      }
    }
    catch (...)
    {
    }
  }

  if (!Recursive)
    return false;

  // check error
  DWORD lastError = GetLastError();
  if (lastError == ERROR_ALREADY_EXISTS)
  {
    // check the attributes
    u32 Attributes = WrapGetFileAttributes(wpath.c_str());
    if (Attributes != INVALID_FILE_ATTRIBUTES && Attributes & FILE_ATTRIBUTE_DIRECTORY)
      return true;
    else
      return false;
  }
  else if (lastError == ERROR_PATH_NOT_FOUND)
  {
    // part of the path does not exist, so we'll create the parent folders, then
    // the full path again.
    const size_t pathLength = wpath.size();
    std::wstring tempPath;
    tempPath.reserve(pathLength);

    // create directories along the path
    for (size_t i = 0; i < pathLength; i++)
    {
      if (wpath[i] == L'\\' || wpath[i] == L'/')
      {
#ifndef _UWP
        const BOOL result = CreateDirectoryW(tempPath.c_str(), nullptr);
#else
        const BOOL result = CreateDirectoryWPtr(tempPath.c_str(), nullptr);
#endif
        if (!result)
        {
          bool isOk = false;
          if (IsValidUWP(convert(tempPath), false))
          {
            try
            {
              auto createTest =
                winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(winrt::hstring(tempPath)).get();
              if (createTest != nullptr)
              {
                isOk = true;
              }
            }
            catch (...)
            {
            }
            if (!isOk)
            {
              try
              {
                std::wstring parent =
                  StringUtil::UTF8StringToWideString(GetFolderParent(StringUtil::WideStringToUTF8String(tempPath)));
                std::wstring name =
                  StringUtil::UTF8StringToWideString(GetFolderName(StringUtil::WideStringToUTF8String(tempPath)));
                auto testFolder =
                  winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(winrt::hstring(parent)).get();
                if (testFolder != nullptr)
                {
                  auto createTest = testFolder
                                      .CreateFolderAsync(winrt::hstring(name),
                                                         winrt::Windows::Storage::CreationCollisionOption::OpenIfExists)
                                      .get();
                  if (createTest != nullptr)
                  {
                    isOk = true;
                  }
                }
              }
              catch (...)
              {
              }
            }
          }

          lastError = GetLastError();
          if (lastError != ERROR_ALREADY_EXISTS && !isOk) // fine, continue to next path segment
            return false;
        }

        // replace / with \.
        tempPath.push_back('\\');
      }
      else
      {
        tempPath.push_back(wpath[i]);
      }
    }

    // re-create the end if it's not a separator, check / as well because windows can interpret them
    if (wpath[pathLength - 1] != L'\\' && wpath[pathLength - 1] != L'/')
    {
#ifndef _UWP
      const BOOL result = CreateDirectoryW(wpath.c_str(), nullptr);
#else
      const BOOL result = CreateDirectoryWPtr(wpath.c_str(), nullptr);
#endif
      if (!result)
      {
        bool isOk = false;
        if (IsValidUWP(convert(wpath), false))
        {
          try
          {
            auto createTest =
              winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(winrt::hstring(wpath)).get();
            if (createTest != nullptr)
            {
              isOk = true;
            }
          }
          catch (...)
          {
          }
          if (!isOk)
          {
            try
            {
              std::wstring parent =
                StringUtil::UTF8StringToWideString(GetFolderParent(StringUtil::WideStringToUTF8String(wpath)));
              std::wstring name =
                StringUtil::UTF8StringToWideString(GetFolderName(StringUtil::WideStringToUTF8String(wpath)));
              auto testFolder =
                winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(winrt::hstring(parent)).get();
              if (testFolder != nullptr)
              {
                auto createTest = testFolder
                                    .CreateFolderAsync(winrt::hstring(name),
                                                       winrt::Windows::Storage::CreationCollisionOption::OpenIfExists)
                                    .get();
                if (createTest != nullptr)
                {
                  isOk = true;
                }
              }
            }
            catch (...)
            {
            }
          }
        }
        lastError = GetLastError();
        if (lastError != ERROR_ALREADY_EXISTS && !isOk)
          return false;
      }
    }

    // ok
    return true;
  }
  else
  {
    // unhandled error
    return false;
  }
}

bool FileSystem::DeleteFile(const char* path)
{
  if (path[0] == '\0')
    return false;

  const std::wstring wpath(StringUtil::UTF8StringToWideString(path));
  const DWORD fileAttributes = WrapGetFileAttributes(wpath.c_str());
  if (fileAttributes == INVALID_FILE_ATTRIBUTES || fileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    return false;

#ifndef _UWP
  return (DeleteFileW(wpath.c_str()) == TRUE);
#else
  bool isOk = false;
  isOk = (DeleteFileWPtr(wpath.c_str()) == TRUE);

  if (!isOk)
  {
    if (IsValidUWP(convert(wpath), false))
    {
      try
      {
        std::string str = convert(wpath);
        resolvePath(str);
        winrt::Windows::Storage::StorageFile testFile = nullptr;

        if (isDataFile(str))
        {
          if (dataFileExists(str))
          {
            testFile = winrt::Windows::Storage::StorageFile::GetFileFromPathAsync(convert(str)).get();
          }
        }
        else
        {
          testFile = winrt::Windows::Storage::StorageFile::GetFileFromPathAsync(convert(str)).get();
        }
        if (testFile != nullptr)
        {
          testFile.DeleteAsync().get();
          isOk = true;
        }
      }
      catch (...)
      {
      }
    }
  }

  return isOk;
#endif
}

bool FileSystem::RenamePath(const char* old_path, const char* new_path)
{
  const std::wstring old_wpath(StringUtil::UTF8StringToWideString(old_path));
  const std::wstring new_wpath(StringUtil::UTF8StringToWideString(new_path));

#ifndef _UWP
  if (!MoveFileExW(old_wpath.c_str(), new_wpath.c_str(), MOVEFILE_REPLACE_EXISTING))
  {
    Log_ErrorPrintf("MoveFileEx('%s', '%s') failed: %08X", old_path, new_path, GetLastError());
    return false;
  }
#else
  // try moving if it doesn't exist, since ReplaceFile fails on non-existing destinations
  if (WrapGetFileAttributes(new_wpath.c_str()) != INVALID_FILE_ATTRIBUTES)
  {
    if (!DeleteFile(new_path))
    {
      Log_ErrorPrintf("DeleteFileW('%s') failed: %08X", new_wpath.c_str(), GetLastError());
      return false;
    }
  }

  if (!MoveFileExWPtr(old_wpath.c_str(), new_wpath.c_str(), 0))
  {
    bool isOk = false;

    winrt::Windows::Storage::IStorageItem testItem = nullptr;
    if (IsValidUWP(std::string(new_path), true) && IsValidUWP(std::string(old_path), true))
    {
      try
      {
        std::wstring parent = StringUtil::UTF8StringToWideString(GetFolderParent(convert(new_wpath)));
        std::wstring name = StringUtil::UTF8StringToWideString(GetFolderName(convert(new_wpath)));
        auto targetFolder =
          winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(winrt::hstring(parent)).get();
        if (targetFolder != nullptr)
        {
          try
          {
            std::string str(old_path);
            resolvePath(str);
            auto testFile = winrt::Windows::Storage::StorageFile::GetFileFromPathAsync(convert(str)).get();
            if (testFile != nullptr)
            {
              try
              {
                testFile
                  .MoveAsync(targetFolder, winrt::hstring(name),
                             winrt::Windows::Storage::NameCollisionOption::ReplaceExisting)
                  .get();
                /*if (newFile != nullptr)
                 {
                   try
                   {
                     testFile.DeleteAsync().get();
                   }
                   catch (...)
                   {
                   }
                 }*/
                isOk = true;
              }
              catch (...)
              {
                testFile
                  .RenameAsync(winrt::hstring(name), winrt::Windows::Storage::NameCollisionOption::ReplaceExisting)
                  .get();
                isOk = true;
              }
            }
          }
          catch (...)
          {
          }

          if (!isOk)
          {
            try
            {
              auto testFolder =
                winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(convert(std::string(old_path))).get();
              if (testFolder != nullptr)
              {
                testFolder.RenameAsync(winrt::hstring(name)).get();
                isOk = true;
              }
            }
            catch (...)
            {
            }
          }
        }
      }
      catch (...)
      {
      }
    }
    if (!isOk)
    {
      Log_ErrorPrintf("MoveFileExW('%s', '%s') failed: %08X", old_path, new_path, GetLastError());
      return false;
    }
  }
#endif

  return true;
}

bool FileSystem::DeleteDirectory(const char* path)
{
  const std::wstring wpath(StringUtil::UTF8StringToWideString(path));
  bool isOk = RemoveDirectoryWPtr(wpath.c_str());
  if (!isOk)
  {
    try
    {
      if (IsValidUWP(std::string(path), false))
      {
        auto testFolder =
          winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(convert(std::string(path))).get();
        if (testFolder != nullptr)
        {
          testFolder.DeleteAsync().get();
          isOk = true;
        }
      }
    }
    catch (...)
    {
    }
  }
  return isOk;
}

std::string FileSystem::GetProgramPath()
{
  std::wstring buffer;
  buffer.resize(MAX_PATH);

  // Fall back to the main module if this fails.
  HMODULE module = nullptr;
#ifndef _UWP
  GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                     reinterpret_cast<LPCWSTR>(&GetProgramPath), &module);
#endif

  for (;;)
  {
    DWORD nChars = GetModuleFileNameW(module, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (nChars == static_cast<DWORD>(buffer.size()) && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
      buffer.resize(buffer.size() * 2);
      continue;
    }

    buffer.resize(nChars);
    break;
  }

  return StringUtil::WideStringToUTF8String(buffer);
}

std::string FileSystem::GetWorkingDirectory()
{
  DWORD required_size = GetCurrentDirectoryW(0, nullptr);
  if (!required_size)
    return {};

  std::wstring buffer;
  buffer.resize(required_size - 1);

  if (!GetCurrentDirectoryW(static_cast<DWORD>(buffer.size() + 1), buffer.data()))
    return {};

  return StringUtil::WideStringToUTF8String(buffer);
}

bool FileSystem::SetWorkingDirectory(const char* path)
{
  const std::wstring wpath(StringUtil::UTF8StringToWideString(path));
  return (SetCurrentDirectoryW(wpath.c_str()) == TRUE);
}

bool FileSystem::SetPathCompression(const char* path, bool enable)
{
  const std::wstring wpath(StringUtil::UTF8StringToWideString(path));
  const DWORD attrs = GetFileAttributesWPtr(wpath.c_str());
  if (attrs == INVALID_FILE_ATTRIBUTES)
    return false;

  const bool isCompressed = (attrs & FILE_ATTRIBUTE_COMPRESSED) != 0;
  if (enable == isCompressed)
  {
    // already compressed/not compressed
    return true;
  }

  const bool isFile = !(attrs & FILE_ATTRIBUTE_DIRECTORY);
  const DWORD flags = isFile ? FILE_ATTRIBUTE_NORMAL : (FILE_FLAG_BACKUP_SEMANTICS | FILE_ATTRIBUTE_DIRECTORY);

#ifndef _UWP
  const HANDLE handle = CreateFileW(wpath.c_str(), FILE_GENERIC_WRITE | FILE_GENERIC_READ,
                                    FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, flags, nullptr);
#else
  const HANDLE handle = CreateFile2Ptr(wpath.c_str(), FILE_GENERIC_WRITE | FILE_GENERIC_READ,
                                       FILE_SHARE_READ | FILE_SHARE_DELETE, OPEN_EXISTING, nullptr);
#endif
  if (handle == INVALID_HANDLE_VALUE)
    return false;

  DWORD bytesReturned = 0;
  DWORD compressMode = enable ? COMPRESSION_FORMAT_DEFAULT : COMPRESSION_FORMAT_NONE;

  bool result = DeviceIoControl(handle, FSCTL_SET_COMPRESSION, &compressMode, 2, nullptr, 0, &bytesReturned, nullptr);

  CloseHandle(handle);
  return result;
}

#else

static u32 RecursiveFindFiles(const char* OriginPath, const char* ParentPath, const char* Path, const char* Pattern,
                              u32 Flags, FileSystem::FindResultsArray* pResults)
{
  std::string tempStr;
  if (Path)
  {
    if (ParentPath)
      tempStr = StringUtil::StdStringFromFormat("%s/%s/%s", OriginPath, ParentPath, Path);
    else
      tempStr = StringUtil::StdStringFromFormat("%s/%s", OriginPath, Path);
  }
  else
  {
    tempStr = StringUtil::StdStringFromFormat("%s", OriginPath);
  }

  DIR* pDir = opendir(tempStr.c_str());
  if (pDir == nullptr)
    return 0;

  // small speed optimization for '*' case
  bool hasWildCards = false;
  bool wildCardMatchAll = false;
  u32 nFiles = 0;
  if (std::strpbrk(Pattern, "*?"))
  {
    hasWildCards = true;
    wildCardMatchAll = (std::strcmp(Pattern, "*") == 0);
  }

  // iterate results
  struct dirent* pDirEnt;
  while ((pDirEnt = readdir(pDir)) != nullptr)
  {
    if (pDirEnt->d_name[0] == '.')
    {
      if (pDirEnt->d_name[1] == '\0' || (pDirEnt->d_name[1] == '.' && pDirEnt->d_name[2] == '\0'))
        continue;

      if (!(Flags & FILESYSTEM_FIND_HIDDEN_FILES))
        continue;
    }

    std::string full_path;
    if (ParentPath != nullptr)
      full_path = StringUtil::StdStringFromFormat("%s/%s/%s/%s", OriginPath, ParentPath, Path, pDirEnt->d_name);
    else if (Path != nullptr)
      full_path = StringUtil::StdStringFromFormat("%s/%s/%s", OriginPath, Path, pDirEnt->d_name);
    else
      full_path = StringUtil::StdStringFromFormat("%s/%s", OriginPath, pDirEnt->d_name);

    FILESYSTEM_FIND_DATA outData;
    outData.Attributes = 0;

#if defined(__HAIKU__) || defined(__APPLE__) || defined(__FreeBSD__)
    struct stat sDir;
    if (stat(full_path.c_str(), &sDir) < 0)
      continue;

#else
    struct stat64 sDir;
    if (stat64(full_path.c_str(), &sDir) < 0)
      continue;
#endif

    if (S_ISDIR(sDir.st_mode))
    {
      if (Flags & FILESYSTEM_FIND_RECURSIVE)
      {
        // recurse into this directory
        if (ParentPath != nullptr)
        {
          std::string recursiveDir = StringUtil::StdStringFromFormat("%s/%s", ParentPath, Path);
          nFiles += RecursiveFindFiles(OriginPath, recursiveDir.c_str(), pDirEnt->d_name, Pattern, Flags, pResults);
        }
        else
        {
          nFiles += RecursiveFindFiles(OriginPath, Path, pDirEnt->d_name, Pattern, Flags, pResults);
        }
      }

      if (!(Flags & FILESYSTEM_FIND_FOLDERS))
        continue;

      outData.Attributes |= FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY;
    }
    else
    {
      if (!(Flags & FILESYSTEM_FIND_FILES))
        continue;
    }

    outData.Size = static_cast<u64>(sDir.st_size);
    outData.CreationTime = sDir.st_ctime;
    outData.ModificationTime = sDir.st_mtime;

    // match the filename
    if (hasWildCards)
    {
      if (!wildCardMatchAll && !StringUtil::WildcardMatch(pDirEnt->d_name, Pattern))
        continue;
    }
    else
    {
      if (std::strcmp(pDirEnt->d_name, Pattern) != 0)
        continue;
    }

    // add file to list
    // TODO string formatter, clean this mess..
    if (!(Flags & FILESYSTEM_FIND_RELATIVE_PATHS))
    {
      outData.FileName = std::move(full_path);
    }
    else
    {
      if (ParentPath != nullptr)
        outData.FileName = StringUtil::StdStringFromFormat("%s/%s/%s", ParentPath, Path, pDirEnt->d_name);
      else if (Path != nullptr)
        outData.FileName = StringUtil::StdStringFromFormat("%s/%s", Path, pDirEnt->d_name);
      else
        outData.FileName = pDirEnt->d_name;
    }

    nFiles++;
    pResults->push_back(std::move(outData));
  }

  closedir(pDir);
  return nFiles;
}

bool FileSystem::FindFiles(const char* Path, const char* Pattern, u32 Flags, FindResultsArray* pResults)
{
  // has a path
  if (Path[0] == '\0')
    return false;

  // clear result array
  if (!(Flags & FILESYSTEM_FIND_KEEP_ARRAY))
    pResults->clear();

  // enter the recursive function
  return (RecursiveFindFiles(Path, nullptr, nullptr, Pattern, Flags, pResults) > 0);
}

bool FileSystem::StatFile(const char* path, struct stat* st)
{
  return stat(path, st) == 0;
}

bool FileSystem::StatFile(std::FILE* fp, struct stat* st)
{
  const int fd = fileno(fp);
  if (fd < 0)
    return false;

  return fstat(fd, st) == 0;
}

bool FileSystem::StatFile(const char* path, FILESYSTEM_STAT_DATA* sd)
{
  // has a path
  if (path[0] == '\0')
    return false;

    // stat file
#if defined(__HAIKU__) || defined(__APPLE__) || defined(__FreeBSD__)
  struct stat sysStatData;
  if (stat(path, &sysStatData) < 0)
#else
  struct stat64 sysStatData;
  if (stat64(path, &sysStatData) < 0)
#endif
    return false;

  // parse attributes
  sd->CreationTime = sysStatData.st_ctime;
  sd->ModificationTime = sysStatData.st_mtime;
  sd->Attributes = 0;
  if (S_ISDIR(sysStatData.st_mode))
    sd->Attributes |= FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY;

  // parse size
  if (S_ISREG(sysStatData.st_mode))
    sd->Size = sysStatData.st_size;
  else
    sd->Size = 0;

  // ok
  return true;
}

bool FileSystem::StatFile(std::FILE* fp, FILESYSTEM_STAT_DATA* sd)
{
  const int fd = fileno(fp);
  if (fd < 0)
    return false;

    // stat file
#if defined(__HAIKU__) || defined(__APPLE__) || defined(__FreeBSD__)
  struct stat sysStatData;
  if (fstat(fd, &sysStatData) < 0)
#else
  struct stat64 sysStatData;
  if (fstat64(fd, &sysStatData) < 0)
#endif
    return false;

  // parse attributes
  sd->CreationTime = sysStatData.st_ctime;
  sd->ModificationTime = sysStatData.st_mtime;
  sd->Attributes = 0;
  if (S_ISDIR(sysStatData.st_mode))
    sd->Attributes |= FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY;

  // parse size
  if (S_ISREG(sysStatData.st_mode))
    sd->Size = sysStatData.st_size;
  else
    sd->Size = 0;

  // ok
  return true;
}

bool FileSystem::FileExists(const char* path)
{
  // has a path
  if (path[0] == '\0')
    return false;

    // stat file
#if defined(__HAIKU__) || defined(__APPLE__) || defined(__FreeBSD__)
  struct stat sysStatData;
  if (stat(path, &sysStatData) < 0)
#else
  struct stat64 sysStatData;
  if (stat64(path, &sysStatData) < 0)
#endif
    return false;

  if (S_ISDIR(sysStatData.st_mode))
    return false;
  else
    return true;
}

bool FileSystem::DirectoryExists(const char* path)
{
  // has a path
  if (path[0] == '\0')
    return false;

    // stat file
#if defined(__HAIKU__) || defined(__APPLE__) || defined(__FreeBSD__)
  struct stat sysStatData;
  if (stat(path, &sysStatData) < 0)
#else
  struct stat64 sysStatData;
  if (stat64(path, &sysStatData) < 0)
#endif
    return false;

  if (S_ISDIR(sysStatData.st_mode))
    return true;
  else
    return false;
}

bool FileSystem::DirectoryIsEmpty(const char* path)
{
  DIR* pDir = opendir(path);
  if (pDir == nullptr)
    return true;

  // iterate results
  struct dirent* pDirEnt;
  while ((pDirEnt = readdir(pDir)) != nullptr)
  {
    if (pDirEnt->d_name[0] == '.')
    {
      if (pDirEnt->d_name[1] == '\0' || (pDirEnt->d_name[1] == '.' && pDirEnt->d_name[2] == '\0'))
        continue;
    }

    closedir(pDir);
    return false;
  }

  closedir(pDir);
  return true;
}

bool FileSystem::CreateDirectory(const char* path, bool recursive)
{
  // has a path
  const size_t pathLength = std::strlen(path);
  if (pathLength == 0)
    return false;

  // try just flat-out, might work if there's no other segments that have to be made
  if (mkdir(path, 0777) == 0)
    return true;

  if (!recursive)
    return false;

  // check error
  int lastError = errno;
  if (lastError == EEXIST)
  {
    // check the attributes
    struct stat sysStatData;
    if (stat(path, &sysStatData) == 0 && S_ISDIR(sysStatData.st_mode))
      return true;
    else
      return false;
  }
  else if (lastError == ENOENT)
  {
    // part of the path does not exist, so we'll create the parent folders, then
    // the full path again.
    std::string tempPath;
    tempPath.reserve(pathLength);

    // create directories along the path
    for (size_t i = 0; i < pathLength; i++)
    {
      if (i > 0 && path[i] == '/')
      {
        if (mkdir(tempPath.c_str(), 0777) < 0)
        {
          lastError = errno;
          if (lastError != EEXIST) // fine, continue to next path segment
            return false;
        }
      }

      tempPath.push_back(path[i]);
    }

    // re-create the end if it's not a separator, check / as well because windows can interpret them
    if (path[pathLength - 1] != '/')
    {
      if (mkdir(path, 0777) < 0)
      {
        lastError = errno;
        if (lastError != EEXIST)
          return false;
      }
    }

    // ok
    return true;
  }
  else
  {
    // unhandled error
    return false;
  }
}

bool FileSystem::DeleteFile(const char* path)
{
  if (path[0] == '\0')
    return false;

  struct stat sysStatData;
  if (stat(path, &sysStatData) != 0 || S_ISDIR(sysStatData.st_mode))
    return false;

  return (unlink(path) == 0);
}

bool FileSystem::RenamePath(const char* old_path, const char* new_path)
{
  if (old_path[0] == '\0' || new_path[0] == '\0')
    return false;

  if (rename(old_path, new_path) != 0)
  {
    Log_ErrorPrintf("rename('%s', '%s') failed: %d", old_path, new_path, errno);
    return false;
  }

  return true;
}

bool FileSystem::DeleteDirectory(const char* path)
{
  if (path[0] == '\0')
    return false;

  struct stat sysStatData;
  if (stat(path, &sysStatData) != 0 || !S_ISDIR(sysStatData.st_mode))
    return false;

  return (unlink(path) == 0);
}

std::string FileSystem::GetProgramPath()
{
#if defined(__linux__)
  static const char* exeFileName = "/proc/self/exe";

  int curSize = PATH_MAX;
  char* buffer = static_cast<char*>(std::realloc(nullptr, curSize));
  for (;;)
  {
    int len = readlink(exeFileName, buffer, curSize);
    if (len < 0)
    {
      std::free(buffer);
      return {};
    }
    else if (len < curSize)
    {
      buffer[len] = '\0';
      std::string ret(buffer, len);
      std::free(buffer);
      return ret;
    }

    curSize *= 2;
    buffer = static_cast<char*>(std::realloc(buffer, curSize));
  }

#elif defined(__APPLE__)

  int curSize = PATH_MAX;
  char* buffer = static_cast<char*>(std::realloc(nullptr, curSize));
  for (;;)
  {
    u32 nChars = curSize - 1;
    int res = _NSGetExecutablePath(buffer, &nChars);
    if (res == 0)
    {
      buffer[nChars] = 0;

      char* resolvedBuffer = realpath(buffer, nullptr);
      if (resolvedBuffer == nullptr)
      {
        std::free(buffer);
        return {};
      }

      std::string ret(buffer);
      std::free(buffer);
      return ret;
    }

    curSize *= 2;
    buffer = static_cast<char*>(std::realloc(buffer, curSize + 1));
  }

#elif defined(__FreeBSD__)
  int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
  char buffer[PATH_MAX];
  size_t cb = sizeof(buffer) - 1;
  int res = sysctl(mib, std::size(mib), buffer, &cb, nullptr, 0);
  if (res != 0)
    return {};

  buffer[cb] = '\0';
  return buffer;
#else
  return {};
#endif
}

std::string FileSystem::GetWorkingDirectory()
{
  std::string buffer;
  buffer.resize(PATH_MAX);
  while (!getcwd(buffer.data(), buffer.size()))
  {
    if (errno != ERANGE)
      return {};

    buffer.resize(buffer.size() * 2);
  }

  buffer.resize(std::strlen(buffer.c_str())); // Remove excess nulls
  return buffer;
}

bool FileSystem::SetWorkingDirectory(const char* path)
{
  return (chdir(path) == 0);
}

bool FileSystem::SetPathCompression(const char* path, bool enable)
{
  return false;
}

#endif
