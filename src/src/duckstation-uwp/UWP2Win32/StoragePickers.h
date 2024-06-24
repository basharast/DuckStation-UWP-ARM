// UWP STORAGE MANAGER
// Copyright (c) 2023 Bashar Astifan.
// Email: bashar@astifan.online
// Telegram: @basharastifan

// This code must keep support for lower builds (15063+)
// Try always to find possible way to keep that support

#pragma once

#include <ppl.h>
#include <ppltasks.h>
#include <string>
#include <vector>

std::string ChooseFolder();
std::string ChooseFile(std::vector<std::string> exts);
