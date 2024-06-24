// Copyright (c) 2023- PPSSPP Project.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0 or later versions.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// A copy of the GPL 2.0 should have been included with the program.
// If not, see http://www.gnu.org/licenses/

// Official git repository and contact information can be found at
// https://github.com/hrydgard/ppsspp and http://www.ppsspp.org/.

#include "pch.h"
#include <fcntl.h>
#include <io.h>
#include <regex>

#include "LaunchItem.h"
#include "StorageExtensions.h"
#include "common/log.h"

#include <ppl.h>
#include <ppltasks.h>

#include "frontend-common/fullscreen_ui.h"

Log_SetChannel(LaunchItem);
extern void AddItemToFutureList(winrt::Windows::Storage::IStorageItem item);
extern bool appReady;
#pragma region LaunchItemClass
class LaunchItem
{
public:
  LaunchItem() {}

  ~LaunchItem() { storageFile = nullptr; }

  void Activate(IStorageItem file)
  {
    storageFile = file;
    AddItemToFutureList(storageFile);
    launchPath = std::string();
    launchOnExit = std::string();
  }

  void Activate(ProtocolActivatedEventArgs args)
  {
    try
    {
      unsigned i;
      winrt::Windows::Foundation::WwwFormUrlDecoder query = args.Uri().QueryParsed();

      for (i = 0; i < query.Size(); i++)
      {
        IWwwFormUrlDecoderEntry arg = query.GetAt(i);

        if (arg.Name() == L"cmd")
        {
          auto command = convert(arg.Value());
          Log_InfoPrintf("Launch command %s", command.c_str());

          std::regex rgx("\"(.+[^\\/]+)\"");
          std::smatch match;

          if (std::regex_search(command, match, rgx))
          {
            try
            {
              launchPath = match[1];
            }
            catch (...)
            {
              launchPath = match[0];
            }
            Log_InfoPrintf("Launch target %s", launchPath.c_str());
          }
        }
        else if (arg.Name() == L"launchOnExit")
        {
          launchOnExit = convert(arg.Value());
          Log_InfoPrintf("On exit URI %s", launchOnExit.c_str());
        }
      }
    }
    catch (...)
    {
    }
    storageFile = nullptr;
  }

  void Start()
  {
    if (IsValid())
    {
      concurrency::create_task([&] {
        SetState(true);
        std::string path = GetFilePath();
        // Delay to be able to launch on startup too
        while (!appReady)
        {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        FullscreenUI::DoStartFile2(path);
      });
    }
  }

  bool IsHandled() { return handled; }
  void SetState(bool fileHandled) { handled = fileHandled; }

  bool IsValid() { return storageFile != nullptr || !launchPath.empty(); }

  std::string GetFilePath()
  {
    std::string path = launchPath;
    if (storageFile != nullptr)
    {
      path = convert(storageFile.Path());
    }
    return path;
  }

  std::string tempURI;
  void Close(bool callLaunchOnExit)
  {
    concurrency::create_task([&] {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      storageFile = nullptr;
      launchPath = std::string();
      handled = false;
      tempURI = std::string(launchOnExit.c_str());
      if (!launchOnExit.empty())
      {
        // if (callLaunchOnExit)
        {
          Log_InfoPrintf("Calling back %s", launchOnExit.c_str());
          winrt::Windows::ApplicationModel::Core::CoreApplication::MainView().CoreWindow().Dispatcher().RunAsync(
            CoreDispatcherPriority::Normal, winrt::Windows::UI::Core::DispatchedHandler([=]() {
              try
              {
                auto target = winrt::hstring(convert(tempURI));
                winrt::Windows::Foundation::Uri uri(target);
                winrt::Windows::System::Launcher::LaunchUriAsync(uri);
              }
              catch (const winrt::hresult_error& ex)
              {
                // Handle the exception here
                Log_ErrorPrintf("Exception occurred during launch: %s", convert(ex.message()).c_str());
              }
              catch (...)
              {
                // Handle any other exceptions here
                Log_ErrorPrintf("Unknown exception occurred during launch");
              }
            }));
        }
        /*else
        {
          Log_InfoPrintf("Ignoring callback %s, due to callLaunchOnExit is false", launchOnExit.c_str());
        }*/
      }
      launchOnExit = std::string();
    });
  }

private:
  IStorageItem storageFile;
  std::string launchPath;
  std::string launchOnExit;
  bool handled = false;
};
#pragma endregion

LaunchItem launchItemHandler;
void DetectLaunchItem(IActivatedEventArgs activateArgs, bool onlyActivate)
{
  if (activateArgs != nullptr)
  {
    if (!launchItemHandler.IsHandled())
    {
      if (activateArgs.Kind() == ActivationKind::File)
      {
        FileActivatedEventArgs fileArgs = activateArgs.as<FileActivatedEventArgs>();
        auto files = fileArgs.Files();
        auto firstFile = files.GetAt(0);
        launchItemHandler.Activate(firstFile);
      }
      else if (activateArgs.Kind() == ActivationKind::Protocol)
      {
        ProtocolActivatedEventArgs protocolArgs = activateArgs.as<ProtocolActivatedEventArgs>();
        launchItemHandler.Activate(protocolArgs);
      }
      if (!onlyActivate)
      {
        launchItemHandler.Start();
      }
    }
  }
}

std::string GetLaunchItemPath(IActivatedEventArgs activateArgs)
{
  DetectLaunchItem(activateArgs, true); // Just activate
  if (launchItemHandler.IsValid())
  {
    // Expected that 'GetLaunchItemPath' called to handle startup item
    // it should be marked as handled by default
    launchItemHandler.SetState(true);
  }
  return launchItemHandler.GetFilePath();
}

void CloseLaunchItem(bool launchOnExit)
{
  if (launchItemHandler.IsValid() && launchItemHandler.IsHandled())
  {
    launchItemHandler.Close(launchOnExit);
  }
}
