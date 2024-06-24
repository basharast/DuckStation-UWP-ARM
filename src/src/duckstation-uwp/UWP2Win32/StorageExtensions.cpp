// UWP STORAGE MANAGER
// Copyright (c) 2023 Bashar Astifan.
// Email: bashar@astifan.online
// Telegram: @basharastifan
// GitHub: https://github.com/basharast/UWP2Win32

#include "StorageExtensions.h"
#include <fcntl.h>

#pragma region Inernal Helpers
std::string make_string(const std::wstring& wstring)
{
  if (wstring.empty())
  {
    return std::string();
  }
  size_t pos;
  size_t begin = 0;
  std::string ret;
  size_t size;
  pos = wstring.find(static_cast<wchar_t>(0), begin);
  while (pos != std::wstring::npos && begin < wstring.length())
  {
    std::wstring segment = std::wstring(&wstring[begin], pos - begin);
    size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, &segment[0], (int)segment.size(), NULL, 0, NULL, NULL);
    std::string converted = std::string(size, 0);
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, &segment[0], (int)segment.size(), &converted[0],
                        (int)converted.size(), NULL, NULL);
    ret.append(converted);
    ret.append({0});
    begin = pos + 1;
    pos = wstring.find(static_cast<wchar_t>(0), begin);
  }
  if (begin <= wstring.length())
  {
    std::wstring segment = std::wstring(&wstring[begin], wstring.length() - begin);
    size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, &segment[0], (int)segment.size(), NULL, 0, NULL, NULL);
    std::string converted = std::string(size, 0);
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, &segment[0], (int)segment.size(), &converted[0],
                        (int)converted.size(), NULL, NULL);
    ret.append(converted);
  }

  return ret;
}

std::wstring make_wstring(const std::string& string)
{
  size_t requiredSize = 0;
  std::wstring answer;
  wchar_t* pWTempString = NULL;

  requiredSize = mbstowcs(NULL, string.c_str(), 0) + 1;

  pWTempString = (wchar_t*)malloc(requiredSize * sizeof(wchar_t));
  if (pWTempString != NULL)
  {
    size_t size = mbstowcs(pWTempString, string.c_str(), requiredSize);
    if (size != (size_t)(-1))
    {
      answer = pWTempString;
    }
  }

  if (pWTempString != NULL)
  {
    free(pWTempString);
  }

  return answer;
}
#pragma endregion

bool replace(std::string& str, const std::string& from, const std::string& to)
{
  size_t start_pos = str.find(from);
  if (start_pos == std::string::npos)
    return false;
  str.replace(start_pos, from.length(), to);
  return true;
}

std::string replace2(const std::string str, const std::string& from, const std::string& to)
{
  std::string cpy = str;
  size_t start_pos = cpy.find(from);
  if (start_pos == std::string::npos)
    return str;
  cpy.replace(start_pos, from.length(), to);
  return cpy;
}

std::vector<std::string> split(const std::string s, char seperator)
{
  std::vector<std::string> output;

  std::string::size_type prev_pos = 0, pos = 0;

  while ((pos = s.find(seperator, pos)) != std::string::npos)
  {
    std::string substring(s.substr(prev_pos, pos - prev_pos));

    output.push_back(substring);

    prev_pos = ++pos;
  }

  output.push_back(s.substr(prev_pos, pos - prev_pos));

  return output;
}

bool isChild(std::string parent, std::string child)
{
  windowsPath(parent);
  windowsPath(child);
  tolower(parent);
  tolower(child);

  return child.find(parent) != std::string::npos;
}

// Parent full path, child full path, child name only
bool isParent(std::string parent, std::string child, std::string childName)
{
  windowsPath(parent);
  windowsPath(child);
  tolower(parent);
  tolower(child);
  tolower(childName);

  parent.append("\\" + childName);
  return parent == child;
}

bool iequals(const std::string a, const std::string b)
{
  auto result = _stricmp(a.c_str(), b.c_str());
  return result == 0;
}

bool equals(const std::string a, const std::string b)
{
  auto result = strcmp(a.c_str(), b.c_str());
  return result == 0;
}

bool ends_with(std::string const& value, std::string const& ending)
{
  if (ending.size() > value.size())
    return false;
  return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

bool starts_with(std::string str, std::string prefix)
{
  return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
}

winrt::hstring convert(const std::string input)
{
  auto wstr = make_wstring(input);
  winrt::hstring output{wstr.c_str()};
  return output;
}

std::wstring convertToWString(const std::string input)
{
  auto wstr = make_wstring(input);
  return wstr;
}

std::string convert(winrt::hstring input)
{
  std::wstring wsstr(input.data());
  auto utf8Str = make_string(wsstr); // UTF8-encoded text

  return utf8Str;
}

std::string convert(std::wstring input)
{
  auto utf8Str = make_string(input); // UTF8-encoded text

  return utf8Str;
}

std::string convert(const char* input)
{
  std::wstring wsstr((const wchar_t*)input);
  auto utf8Str = make_string(wsstr); // UTF8-encoded text

  return utf8Str;
}

LPCWSTR convertToLPCWSTR(std::string input)
{
  std::wstring stemp = make_wstring(input);
  LPCWSTR sw = stemp.c_str();

  return sw;
}

LPCWSTR convertToLPCWSTR(winrt::hstring input)
{
  std::string inputString = convert(input);
  std::wstring stemp = make_wstring(inputString);
  LPCWSTR sw = stemp.c_str();

  return sw;
}

const char* convertToChar(winrt::hstring input)
{
  std::string output = convert(input);
  return output.c_str();
}

void tolower(std::string& input) {}

void tolower(winrt::hstring& input)
{
  std::string temp = convert(input);
  tolower(temp);
  input = convert(temp);
}

void toupper(std::string& input) {}

void toupper(winrt::hstring& input)
{
  std::string temp = convert(input);
  toupper(temp);
  input = convert(temp);
}

void windowsPath(std::string& path)
{
  std::replace(path.begin(), path.end(), '/', '\\');
}

void windowsPath(winrt::hstring& path)
{
  std::string temp = convert(path);
  windowsPath(temp);
  path = convert(temp);
}

std::string merge(std::string targetFullPath, std::string subFullPath)
{
  size_t pos = subFullPath.rfind('\\');
  if (pos != std::string::npos)
  {
    auto itemName = subFullPath.substr(pos + 1);
    std::string newPath = targetFullPath + "\\" + itemName;
    return newPath;
  }
  return targetFullPath;
}

std::string& rtrim(std::string& s, const char* t)
{
  s.erase(s.find_last_not_of(t) + 1);
  return s;
}

// trim from beginning of string (left)
std::string& ltrim(std::string& s, const char* t)
{
  s.erase(0, s.find_first_not_of(t));
  return s;
}

// trim from both ends of string (right then left)
std::string& trim(std::string& s, const char* t)
{
  return ltrim(rtrim(s, t), t);
}

FILE_OPEN_UWP_MODE* GetFileMode(const char* mode)
{
  std::string tempMode = std::string(mode);
  replace(tempMode, "S", "");
  replace(tempMode, "R", "");
  const char* modeClean = tempMode.c_str();
  size_t size = sizeof(FILE_OPEN_UWP_MODE);
  FILE_OPEN_UWP_MODE* openMode = (FILE_OPEN_UWP_MODE*)(malloc(size));

  if (openMode)
  {
    openMode->dwDesiredAccess = GENERIC_READ;
    openMode->dwShareMode = FILE_SHARE_READ;
    openMode->dwCreationDisposition = OPEN_EXISTING;
    openMode->flags = 0;
    openMode->isWrite = false;
    openMode->isAppend = false;
    openMode->isCreate = false;

    if (!strcmp(modeClean, "r") || !strcmp(modeClean, "rb") || !strcmp(modeClean, "rt"))
    {
      openMode->dwDesiredAccess = GENERIC_READ;
      openMode->dwShareMode = FILE_SHARE_READ;
      openMode->dwCreationDisposition = OPEN_EXISTING;
      openMode->flags = _O_RDONLY;
    }
    else if (!strcmp(modeClean, "r+") || !strcmp(modeClean, "rb+") || !strcmp(modeClean, "r+b") || !strcmp(modeClean, "rt+") ||
             !strcmp(modeClean, "r+t"))
    {
      openMode->dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
      openMode->dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
      openMode->dwCreationDisposition = OPEN_EXISTING;
      openMode->flags = _O_RDWR;
      openMode->isWrite = true;
    }
    else if (!strcmp(modeClean, "a") || !strcmp(modeClean, "ab") || !strcmp(modeClean, "at"))
    {
      openMode->dwDesiredAccess = GENERIC_WRITE;
      openMode->dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
      openMode->dwCreationDisposition = OPEN_ALWAYS;
      openMode->flags = _O_APPEND | _O_WRONLY | _O_CREAT;
      openMode->isWrite = true;
      openMode->isCreate = true;
      openMode->isAppend = true;
    }
    else if (!strcmp(modeClean, "a+") || !strcmp(modeClean, "ab+") || !strcmp(modeClean, "a+b") || !strcmp(modeClean, "at+") ||
             !strcmp(modeClean, "a+t"))
    {
      openMode->dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
      openMode->dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
      openMode->dwCreationDisposition = OPEN_ALWAYS;
      openMode->flags = _O_APPEND | _O_RDWR | _O_CREAT;
      openMode->isWrite = true;
      openMode->isCreate = true;
      openMode->isAppend = true;
    }
    else if (!strcmp(modeClean, "w") || !strcmp(modeClean, "wb") || !strcmp(modeClean, "wt"))
    {
      openMode->dwDesiredAccess = GENERIC_WRITE;
      openMode->dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
      openMode->dwCreationDisposition = CREATE_ALWAYS;
      openMode->flags = _O_WRONLY | _O_CREAT | _O_TRUNC;
      openMode->isWrite = true;
      openMode->isCreate = true;
    }
    else if (!strcmp(modeClean, "w+") || !strcmp(modeClean, "wb+") || !strcmp(modeClean, "w+b") || !strcmp(modeClean, "wt+") ||
             !strcmp(modeClean, "w+t"))
    {
      openMode->dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
      openMode->dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
      openMode->dwCreationDisposition = CREATE_ALWAYS;
      openMode->flags = _O_RDWR | _O_CREAT | _O_TRUNC;
      openMode->isWrite = true;
      openMode->isCreate = true;
    }

    if (strpbrk(mode, "t") != nullptr)
    {
      openMode->flags |= _O_TEXT;
    }
    if (strpbrk(mode, "b") != nullptr)
    {
      openMode->flags |= _O_BINARY;
    }
  }
  return openMode;
}

// Parent and child full path
std::string getSubRoot(std::string parent, std::string child)
{
  windowsPath(parent);
  windowsPath(child);
  auto childCut = child;
  replace(childCut, (parent + "\\"), "");
  size_t len = childCut.find_first_of('\\', 0);
  auto subRoot = childCut.substr(0, len);

  return parent + "\\" + subRoot;
}
