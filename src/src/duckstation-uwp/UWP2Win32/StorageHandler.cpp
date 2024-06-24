// UWP STORAGE MANAGER
// Copyright (c) 2023 Bashar Astifan.
// Email: bashar@astifan.online
// Telegram: @basharastifan

#include "StorageHandler.h"
#include <common/log.h>

HRESULT GetFileHandle(winrt::Windows::Storage::StorageFile file, HANDLE* handle, HANDLE_ACCESS_OPTIONS accessMode,
                      HANDLE_SHARING_OPTIONS shareMode)
{
  if (file != nullptr)
  {
    Microsoft::WRL::ComPtr<IUnknown> abiPointer(reinterpret_cast<IUnknown*>(winrt::get_abi(file)));
    Microsoft::WRL::ComPtr<IStorageItemHandleAccess> handleAccess;
    if (SUCCEEDED(abiPointer.As(&handleAccess)))
    {
      HANDLE hFile = INVALID_HANDLE_VALUE;

      if (SUCCEEDED(handleAccess->Create(accessMode, shareMode, HO_NONE, nullptr, &hFile)))
      {
        *handle = hFile;
        return S_OK;
      }
    }
  }
  return E_FAIL;
}

HRESULT GetFolderHandle(winrt::Windows::Storage::StorageFolder folder, HANDLE* handle, HANDLE_ACCESS_OPTIONS accessMode,
                        HANDLE_SHARING_OPTIONS shareMode)
{
  if (folder != nullptr)
  {
    Microsoft::WRL::ComPtr<IUnknown> abiPointer(reinterpret_cast<IUnknown*>(winrt::get_abi(folder)));
    Microsoft::WRL::ComPtr<IStorageItemHandleAccess> handleAccess;
    if (SUCCEEDED(abiPointer.As(&handleAccess)))
    {
      HANDLE hFolder = INVALID_HANDLE_VALUE;

      if (SUCCEEDED(handleAccess->Create(accessMode, shareMode, HO_NONE, nullptr, &hFolder)))
      {
        *handle = hFolder;
        return S_OK;
      }
    }
  }
  return E_FAIL;
}

Log_SetChannel(FileSystem);
HANDLE_ACCESS_OPTIONS GetAccessMode(int accessMode)
{
  int accessModeOut = 0;
  if (accessMode & GENERIC_READ)
  {
    accessModeOut |= (HAO_READ | HAO_READ_ATTRIBUTES);
    Log_WarningPrint("GetAccessMode + HAO_READ | HAO_READ_ATTRIBUTES");
  }
  if (accessMode & GENERIC_WRITE)
  {
    accessModeOut |= HAO_WRITE;
    Log_WarningPrint("GetAccessMode + HAO_WRITE");
  }
  if (accessMode & GENERIC_ALL)
  {
    accessModeOut |= (HAO_READ | HAO_READ_ATTRIBUTES | HAO_WRITE | HAO_DELETE);
    Log_WarningPrint("GetAccessMode + HAO_READ | HAO_READ_ATTRIBUTES | HAO_WRITE | HAO_DELETE");
  }

  return (HANDLE_ACCESS_OPTIONS)accessModeOut;
}

HANDLE_SHARING_OPTIONS GetShareMode(int shareMode)
{
  int shareModeOut = 0;
  if (shareMode & FILE_SHARE_READ)
  {
    shareModeOut |= HSO_SHARE_READ;
    Log_WarningPrint("GetShareMode + HSO_SHARE_READ");
  }
  if (shareMode & FILE_SHARE_WRITE)
  {
    shareModeOut |= HSO_SHARE_WRITE;
    Log_WarningPrint("GetShareMode + HSO_SHARE_WRITE");
  }
  if (shareMode & FILE_SHARE_DELETE)
  {
    shareModeOut |= HSO_SHARE_DELETE;
    Log_WarningPrint("GetShareMode + HSO_SHARE_DELETE");
  }

  return (HANDLE_SHARING_OPTIONS)shareModeOut;
}

HANDLE_CREATION_OPTIONS GetOpenMode(int openMode)
{
  switch (openMode)
  {
    case CREATE_NEW:
    {
      Log_WarningPrint("GetOpenMode + HCO_CREATE_NEW");
      return HCO_CREATE_NEW;
    }
    case CREATE_ALWAYS:
    {
      Log_WarningPrint("GetOpenMode + HCO_CREATE_ALWAYS");
      return HCO_CREATE_ALWAYS;
    }
    case OPEN_ALWAYS:
    {
      Log_WarningPrint("GetOpenMode + HCO_OPEN_ALWAYS");
      return HCO_OPEN_ALWAYS;
    }
    case OPEN_EXISTING:
    {
      Log_WarningPrint("GetOpenMode + HCO_OPEN_EXISTING");
      return HCO_OPEN_EXISTING;
    }
    default:
    {
      Log_WarningPrint("GetOpenMode + HCO_OPEN_EXISTING");
      return HCO_OPEN_EXISTING;
    }
  }
}
