/*
 *  Copyright (C) 2009-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// The times in comments are how much time was spent parsing
// the header file according to C++ Build Insights in VS2019
#define _CRT_RAND_S
#include <algorithm> // 32 seconds
#include <chrono>    // 72 seconds
#include <fstream>
#include <iostream>
#include <map>
#include <mutex> // 19 seconds
#include <string>
#include <vector>

#include <fmt/core.h>
#include <fmt/format.h> // 53 seconds
#include <intrin.h>     // 97 seconds
#include <ppltasks.h>   // 87seconds, not included by us
#include <unknwn.h>

#include <TCHAR.H>
#include <comdef.h>
#include <locale>
#include <memory>
#include <windows.h>

#include <WinRTBase.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.ApplicationModel.Activation.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Display.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.System.Threading.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.UI.Input.h>
#include <winrt/Windows.UI.ViewManagement.h>