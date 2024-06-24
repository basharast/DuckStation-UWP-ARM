// UWP STORAGE MANAGER
// Copyright (c) 2023 Bashar Astifan.
// Email: bashar@astifan.online
// Telegram: @basharastifan
// GitHub: https://github.com/basharast/UWP2Win32

// This is small bridge to invoke UWP Storage manager

// This code must keep support for lower builds (15063+)
// Try always to find possible way to keep that support

#pragma once
#ifdef __cplusplus
extern "C" {
#endif
void* _fopenuwp_(const char* path, const char* mode);
void* mallocUWP(void* no, const size_t size, const int reserve, const int access);
void* reallocUWP(void* old_ptr, size_t old_size, size_t new_size);
#ifdef __cplusplus
}
#endif
