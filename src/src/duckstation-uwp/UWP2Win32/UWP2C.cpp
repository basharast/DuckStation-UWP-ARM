// UWP STORAGE MANAGER
// Copyright (c) 2023 Bashar Astifan.
// Email: bashar@astifan.online
// Telegram: @basharastifan
// GitHub: https://github.com/basharast/UWP2Win32

// This is small bridge to invoke UWP Storage manager

// This code must keep support for lower builds (15063+)
// Try always to find possible way to keep that support

#include "UWP2C.h"
#include "common/byte_stream.h"
#include "common/file_system.h"
#include <StorageExtensions.h>
#include <string>

extern LPVOID(WINAPI* VirtualAllocPtr)(LPVOID, SIZE_T, DWORD, DWORD);
;

#ifdef __cplusplus
extern "C" {
#endif
void* _fopenuwp_(const char* path, const char* mode)
{
  std::string pathString = convert(path);
  return FileSystem::OpenCFile(path, mode);
}
void* mallocUWP(void* no, const size_t size, const int reserve, const int access)
{
  return VirtualAllocPtr(no, size, reserve, access);
}

void* reallocUWP(void* old_ptr, size_t old_size, size_t new_size)
{
  if (new_size == 0)
  {
    free(old_ptr);
    return NULL;
  }

  void* new_ptr = malloc(new_size);
  if (new_ptr == NULL)
  {
    return NULL; // Allocation failed
  }

  if (old_ptr != NULL)
  {
    // Copy old data into the new block.
    // Note: we use the smaller of old_size and new_size
    size_t size_to_copy = old_size < new_size ? old_size : new_size;
    memcpy(new_ptr, old_ptr, size_to_copy);

    // Free the old block
    free(old_ptr);
  }

  return new_ptr;
}
#ifdef __cplusplus
}
#endif
