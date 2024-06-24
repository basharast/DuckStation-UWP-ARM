#include "uwp_host_interface.h"
#include "IconsFontAwesome5.h"
#include "common/assert.h"
#include "common/byte_stream.h"
#include "common/file_system.h"
#include "common/log.h"
#include "common/string_util.h"
#include "core/controller.h"
#include "core/gpu.h"
#include "core/host_display.h"
#include "core/host_interface.h"
#include "core/imgui_fullscreen.h"
#include "core/imgui_styles.h"
#include "core/settings.h"
#include "core/system.h"
#include "frontend-common/controller_interface.h"
#include "frontend-common/d3d11_host_display.h"
#include "frontend-common/d3d12_host_display.h"
#include "frontend-common/fullscreen_ui.h"
#include "frontend-common/icon.h"
#include "frontend-common/ini_settings_interface.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"
#include "uwp_key_map.h"
#include <cinttypes>
#include <cmath>
#include <mutex>
#include <ppltasks.h>
Log_SetChannel(UWPHostInterface);

#include <gamingdeviceinformation.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Display.Core.h>
#include <winrt/Windows.Storage.Pickers.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.System.Profile.h>

#include "UWP2Win32/StorageExtensions.h"
#include "UWP2Win32/StorageWin32.h"
#include <LaunchItem.h>
#include <concrt.h>

using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Gaming::Input;
using namespace winrt::Windows::Devices::Sensors;

std::string appTitle = "";

bool appReady = false;
bool isResizing = false;
bool input_changed = false;
bool isLandscape;
bool m_windowVisible;
bool cacheState = true;
float touchThreshold = 2.0f; // Experiment with the value
u32 shader_opt = 0;
float targetUpdateTime = 60.0f;
double targetFrameTime = 1.0 / 60.0; // 30 FPS

extern bool s_quick_menu_was_open;
winrt::Windows::Graphics::Display::DisplayOrientations currentOrientation;

int screenScale;

struct ButtonCustomMap
{
  int index = 0;
  GamepadButtons gaming = GamepadButtons::None;
  winrt::Windows::System::VirtualKey native = winrt::Windows::System::VirtualKey::None;
  int raw = 0;
  int direct = 0;
};

ButtonCustomMap AButton;
ButtonCustomMap BButton;
ButtonCustomMap XButton;
ButtonCustomMap YButton;
ButtonCustomMap R1Button;
ButtonCustomMap L1Button;
ButtonCustomMap UpButton;
ButtonCustomMap DownButton;
ButtonCustomMap LeftButton;
ButtonCustomMap RightButton;

ButtonCustomMap L2Button;
ButtonCustomMap R2Button;
ButtonCustomMap L3Button;
ButtonCustomMap R3Button;

ButtonCustomMap StartButton;
ButtonCustomMap SelectButton;

ButtonCustomMap Button0 = {0, GamepadButtons::A, winrt::Windows::System::VirtualKey::GamepadA};
ButtonCustomMap Button1 = {1, GamepadButtons::B, winrt::Windows::System::VirtualKey::GamepadB};
ButtonCustomMap Button2 = {2, GamepadButtons::X, winrt::Windows::System::VirtualKey::GamepadX};
ButtonCustomMap Button3 = {3, GamepadButtons::Y, winrt::Windows::System::VirtualKey::GamepadY};
ButtonCustomMap Button4 = {4, GamepadButtons::RightShoulder, winrt::Windows::System::VirtualKey::GamepadRightShoulder};
ButtonCustomMap Button5 = {5, GamepadButtons::LeftShoulder, winrt::Windows::System::VirtualKey::GamepadLeftShoulder};
ButtonCustomMap Button6 = {6, GamepadButtons::DPadUp, winrt::Windows::System::VirtualKey::GamepadDPadUp};
ButtonCustomMap Button7 = {7, GamepadButtons::DPadDown, winrt::Windows::System::VirtualKey::GamepadDPadDown};
ButtonCustomMap Button8 = {8, GamepadButtons::DPadLeft, winrt::Windows::System::VirtualKey::GamepadDPadLeft};
ButtonCustomMap Button9 = {9, GamepadButtons::DPadRight, winrt::Windows::System::VirtualKey::GamepadDPadRight};

ButtonCustomMap Button10 = {10, GamepadButtons::None, winrt::Windows::System::VirtualKey::GamepadLeftTrigger, 0, -1};
ButtonCustomMap Button11 = {11, GamepadButtons::None, winrt::Windows::System::VirtualKey::GamepadRightTrigger, 0, -2};
ButtonCustomMap Button12 = {12, GamepadButtons::LeftThumbstick,
                            winrt::Windows::System::VirtualKey::GamepadLeftThumbstickButton};
ButtonCustomMap Button13 = {13, GamepadButtons::RightThumbstick,
                            winrt::Windows::System::VirtualKey::GamepadRightThumbstickButton};

ButtonCustomMap Button14 = {14, GamepadButtons::Menu, winrt::Windows::System::VirtualKey::GamepadMenu};
ButtonCustomMap Button15 = {15, GamepadButtons::View, winrt::Windows::System::VirtualKey::GamepadView};

void UpdateTargetButton(ButtonCustomMap& target, u32 index)
{
  switch (index)
  {
    case 0:
      target = Button0;
      break;
    case 1:
      target = Button1;
      break;
    case 2:
      target = Button2;
      break;
    case 3:
      target = Button3;
      break;
    case 4:
      target = Button4;
      break;
    case 5:
      target = Button5;
      break;
    case 6:
      target = Button6;
      break;
    case 7:
      target = Button7;
      break;
    case 8:
      target = Button8;
      break;
    case 9:
      target = Button9;
      break;
    case 10:
      target = Button10;
      break;
    case 11:
      target = Button11;
      break;
    case 12:
      target = Button12;
      break;
    case 13:
      target = Button13;
      break;
    case 14:
      target = Button14;
      break;
    case 15:
      target = Button15;
      break;
  }
}

void updateButtonsMap()
{
  if (g_settings.gaming_map)
  {
    UpdateTargetButton(AButton, g_settings.gaming_custom_cross);
    UpdateTargetButton(BButton, g_settings.gaming_custom_circle);
    UpdateTargetButton(XButton, g_settings.gaming_custom_square);
    UpdateTargetButton(YButton, g_settings.gaming_custom_train);
    UpdateTargetButton(L1Button, g_settings.gaming_custom_l1);
    UpdateTargetButton(R1Button, g_settings.gaming_custom_r1);
    UpdateTargetButton(UpButton, g_settings.gaming_custom_up);
    UpdateTargetButton(DownButton, g_settings.gaming_custom_down);
    UpdateTargetButton(LeftButton, g_settings.gaming_custom_left);
    UpdateTargetButton(RightButton, g_settings.gaming_custom_right);
    UpdateTargetButton(L2Button, g_settings.gaming_custom_l2);
    UpdateTargetButton(R1Button, g_settings.gaming_custom_r2);
    UpdateTargetButton(L3Button, g_settings.gaming_custom_l3);
    UpdateTargetButton(R3Button, g_settings.gaming_custom_r3);
    UpdateTargetButton(StartButton, g_settings.gaming_custom_start);
    UpdateTargetButton(SelectButton, g_settings.gaming_custom_select);
  }
  else
  {
    UpdateTargetButton(AButton, 0);
    UpdateTargetButton(BButton, 1);
    UpdateTargetButton(XButton, 2);
    UpdateTargetButton(YButton, 3);
    UpdateTargetButton(L1Button, 4);
    UpdateTargetButton(R1Button, 5);
    UpdateTargetButton(UpButton, 6);
    UpdateTargetButton(DownButton, 7);
    UpdateTargetButton(LeftButton, 8);
    UpdateTargetButton(RightButton, 9);
    UpdateTargetButton(L2Button, 10);
    UpdateTargetButton(R1Button, 11);
    UpdateTargetButton(L3Button, 12);
    UpdateTargetButton(R3Button, 13);
    UpdateTargetButton(StartButton, 14);
    UpdateTargetButton(SelectButton, 15);
  }

  touchThreshold = 10.0f - g_settings.touch_sens;
  cacheState = g_settings.cache_shader;
  shader_opt = g_settings.shader_opt;
  targetUpdateTime = g_settings.display_max_fps;
  if (targetUpdateTime < 5.0f)
  {
    targetUpdateTime = 60.0f;
  }
  targetFrameTime = 1.0 / (double)targetUpdateTime;
}

// Declare a Gamepad object
std::list<Gamepad> myGamepads;
#ifndef NO_RAW
std::list<RawGameController> myRawGamepads;
#endif
concurrency::critical_section myLock{};
winrt::event_token gamepadAddedToken;
winrt::event_token gamepadRemovedToken;

void UWPHostInterface::GamePadAddedEventHandler(winrt::Windows::Foundation::IInspectable const& sender,
                                                winrt::Windows::Gaming::Input::Gamepad const& gamepad)
{
  input_changed = true;
  Log_WarningPrintf("GamePadAddedEventHandler");
  InitializeGamepad();
  // UpdateInputMap();
}

void UWPHostInterface::GamePadRemovedEventHandler(winrt::Windows::Foundation::IInspectable const& sender,
                                                  winrt::Windows::Gaming::Input::Gamepad const& gamepad)
{
  input_changed = true;
  Log_WarningPrintf("GamePadRemovedEventHandler");
  InitializeGamepad();
  // UpdateInputMap();
}

void UWPHostInterface::RegisterGamepadEvents()
{
  gamepadAddedToken =
    winrt::Windows::Gaming::Input::Gamepad::GamepadAdded({this, &UWPHostInterface::GamePadAddedEventHandler});
  gamepadAddedToken =
    winrt::Windows::Gaming::Input::Gamepad::GamepadRemoved({this, &UWPHostInterface::GamePadRemovedEventHandler});
}

void UWPHostInterface::UnregisterGamepadEvents()
{
  try
  {
    winrt::Windows::Gaming::Input::Gamepad::GamepadAdded(gamepadAddedToken);
    winrt::Windows::Gaming::Input::Gamepad::GamepadRemoved(gamepadRemovedToken);
  }
  catch (...)
  {
  }
}

// Initialize the Gamepad object
bool isEventRegistered = false;
void UWPHostInterface::InitializeGamepad()
{
  if (!isEventRegistered)
  {
    RegisterGamepadEvents();

    isEventRegistered = true;
  }

  myGamepads.clear();

#ifndef NO_RAW
  myRawGamepads.clear();
#endif

  for (auto const& gamepad : Gamepad::Gamepads())
  {
    // Test whether the gamepad is already in myGamepads; if it isn't, add it.
    concurrency::critical_section::scoped_lock lock{myLock};
    myGamepads.push_back(gamepad);
  }

#ifndef NO_RAW
  for (auto const& gamepad : RawGameController::RawGameControllers())
  {
    // Test whether the gamepad is already in myGamepads; if it isn't, add it.
    concurrency::critical_section::scoped_lock lock{myLock};
    myRawGamepads.push_back(gamepad);
  }
#endif
  Log_WarningPrintf("InitializeGamepad: total = %d", myGamepads.size());
#ifndef NO_RAW
  Log_WarningPrintf("InitializeGamepad (Raw): total = %d", myRawGamepads.size());
#endif
  input_changed = false;
}

bool AccelerometerReady = false;
bool ReadingInProgress = false;

bool leftState = false;
bool rightState = false;
bool upState = false;
bool downState = false;

void UWPHostInterface::CallRight(bool state)
{
  // if (state != rightState)
  {
    int port = g_settings.touch_port - 1;
    TriggerButton(port, (int)ButtonsX::Right, state);
    rightState = state;
  }
}

void UWPHostInterface::CallLeft(bool state)
{
  // if (state != leftState)
  {
    int port = g_settings.touch_port - 1;
    TriggerButton(port, (int)ButtonsX::Left, state);
    leftState = state;
  }
}
void UWPHostInterface::CallUp(bool state)
{
  // if (state != upState)
  {
    int port = g_settings.touch_port - 1;
    TriggerButton(port, (int)ButtonsX::Up, state);
    upState = state;
  }
}
void UWPHostInterface::CallDown(bool state)
{
  // if (state != downState)
  {
    int port = g_settings.touch_port - 1;
    TriggerButton(port, (int)ButtonsX::Down, state);
    downState = state;
  }
}
void UWPHostInterface::OnReadingChanged(
  winrt::Windows::Devices::Sensors::Accelerometer const& sender,
  winrt::Windows::Devices::Sensors::AccelerometerReadingChangedEventArgs const& args)
{
  if (System::IsShutdown() || System::IsPaused() || s_quick_menu_was_open || !g_settings.sensors_enable ||
      ReadingInProgress)
  {
    return;
  }

  try
  {
    ReadingInProgress = true;
    double xAxisAcc = 0;
    double yAxisAcc = 0;
    double zAxisAcc = 0;
    AccelerometerReading reading = sender.GetCurrentReading();
    switch (currentOrientation)
    {
      case winrt::Windows::Graphics::Display::DisplayOrientations::Landscape:
      {
        xAxisAcc = reading.AccelerationX() * 10.0;
        yAxisAcc = reading.AccelerationY() * 10.0;
        zAxisAcc = reading.AccelerationZ() * 10.0;
      }
      break;
      case winrt::Windows::Graphics::Display::DisplayOrientations::Portrait:
      {
        xAxisAcc = reading.AccelerationY() * 10.0;
        yAxisAcc = (-1 * reading.AccelerationX()) * 10.0;
        zAxisAcc = reading.AccelerationZ() * 10.0;
      }
      break;
      case winrt::Windows::Graphics::Display::DisplayOrientations::LandscapeFlipped:
      {
        xAxisAcc = (-1 * reading.AccelerationX()) * 10.0;
        yAxisAcc = (-1 * reading.AccelerationY()) * 10.0;
        zAxisAcc = reading.AccelerationZ() * 10.0;
      }
      break;
      case winrt::Windows::Graphics::Display::DisplayOrientations::PortraitFlipped:
      {
        xAxisAcc = (-1 * reading.AccelerationY()) * 10.0;
        yAxisAcc = reading.AccelerationX() * 10.0;
        zAxisAcc = reading.AccelerationZ() * 10.0;
      }
      break;
    }

    try
    {
      if (yAxisAcc <= -((double)g_settings.sensors_sens))
      {
        CallRight();
      }
      else
      {
        CallRight(false);
      }

      if (yAxisAcc >= ((double)g_settings.sensors_sens))
      {
        CallLeft();
      }
      else
      {
        CallLeft(false);
      }

      if (g_settings.sensors_up)
      {
        if (xAxisAcc >= -(g_settings.sensors_sens))
        {
          CallUp();
        }
        else
        {
          CallUp(false);
        }
      }
      if (g_settings.sensors_down)
      {
        if (xAxisAcc <= -(g_settings.sensors_sens + 1.0))
        {
          CallDown();
        }
        else
        {
          CallDown(false);
        }
      }
    }
    catch (...)
    {
    }

    ReadingInProgress = false;
  }
  catch (...)
  {
  }
}

winrt::Windows::Devices::Sensors::Accelerometer _accelerometer = Accelerometer::GetDefault();

void UWPHostInterface::LinkAccelerometer()
{
#if 1
  if (_accelerometer != nullptr && !AccelerometerReady)
  {
    // Establish the report interval
    UINT minReportInterval = _accelerometer.MinimumReportInterval();
    UINT reportInterval = minReportInterval > 1 ? minReportInterval : 1;
    _accelerometer.ReportInterval(reportInterval);

    // Assign an event handler for the reading-changed event
    _accelerometer.ReadingChanged({this, &UWPHostInterface::OnReadingChanged});
    AccelerometerReady = true;
  }
#endif
}

static bool IsRunningOnXbox()
{
  const auto version_info = winrt::Windows::System::Profile::AnalyticsInfo::VersionInfo();
  const auto device_family = version_info.DeviceFamily();
  return (device_family == L"Windows.Xbox");
}

UWPHostInterface::UWPHostInterface() = default;

UWPHostInterface::~UWPHostInterface()
{
  FullscreenUI::DoPowerOff();
  m_display->DestroyRenderDevice();
}

winrt::Windows::ApplicationModel::Core::IFrameworkView UWPHostInterface::CreateView()
{
  return *this;
}

#include <iostream>
#include <winrt/Windows.ApplicationModel.h>

void getAppVersion()
{
  winrt::Windows::ApplicationModel::Package package = winrt::Windows::ApplicationModel::Package::Current();
  winrt::Windows::ApplicationModel::PackageVersion version = package.Id().Version();

  std::wstring appName = package.DisplayName().c_str();
  std::wstring appVersion =
    std::to_wstring(version.Major) + L"." + std::to_wstring(version.Minor) + L"." + std::to_wstring(version.Build);

  appTitle.clear();
  appTitle.append(convert(appName));
  appTitle.append(" ");
  appTitle.append(convert(appVersion));
}

bool appSuspended = false;
bool appClosed = false;
void UWPHostInterface::Initialize(const winrt::Windows::ApplicationModel::Core::CoreApplicationView& a)
{
  winrt::Windows::ApplicationModel::Core::CoreApplication::Suspending({this, &UWPHostInterface::OnSuspending});
  winrt::Windows::ApplicationModel::Core::CoreApplication::Resuming({this, &UWPHostInterface::OnResuming});
  a.Activated({this, &UWPHostInterface::OnActivated});
}

void UWPHostInterface::Load(const winrt::hstring&) {}

void UWPHostInterface::Uninitialize() {}

const char* UWPHostInterface::GetFrontendName() const
{
  return "DuckStation UWP Frontend";
}

bool UWPHostInterface::Initialize()
{
  Log::SetDebugOutputParams(true, nullptr, LOGLEVEL_DEBUG);
  if (!SetDirectories())
    return false;

  LinkWin32APIs();
  getAppVersion();

  m_settings_interface = std::make_unique<INISettingsInterface>(GetSettingsFileName());
  m_flags.force_fullscreen_ui = true;
  m_fullscreen_ui_enabled = true;

  if (!CommonHostInterface::Initialize())
    return false;

  SetImGuiKeyMap();

  const bool start_fullscreen = m_flags.start_fullscreen || g_settings.start_fullscreen;
  if (!CreateDisplay(start_fullscreen))
  {
    Log_ErrorPrintf("Failed to create host display");
    return false;
  }
  appReady = true;
  m_windowVisible = true;

  InitializeGamepad();
  updateButtonsMap();
  LinkAccelerometer();
  return true;
}

void UWPHostInterface::Shutdown()
{
  UnregisterGamepadEvents();
  DestroyDisplay();

  CommonHostInterface::Shutdown();
}

void UWPHostInterface::SetDefaultSettings(SettingsInterface& si)
{
  CommonHostInterface::SetDefaultSettings(si);

  si.SetStringValue("CPU", "FastmemMode", "LUT");
  si.SetStringValue("Main", "ControllerBackend", "XInput");
  si.AddToStringList("GameList", "RecursivePaths", GetUserDirectoryRelativePath("games").c_str());

  // TODO: Maybe we should bind this to F1 in the future.
  si.SetStringValue("Hotkeys", "OpenQuickMenu", "Keyboard/Escape");
#if _M_ARM || defined(_M_ARM64) 
  SetDefaultSettingsForARM(si);
#else
  SetDefaultSettingsForXbox(si);
#endif
}

extern bool isLevel93;
extern bool s_compatibility_window_open;

bool UWPHostInterface::CreateDisplay(bool fullscreen)
{
  Assert(!m_display);

  m_appview = winrt::Windows::UI::ViewManagement::ApplicationView::GetForCurrentView();
  m_appview.PreferredLaunchWindowingMode(
    fullscreen ? winrt::Windows::UI::ViewManagement::ApplicationViewWindowingMode::FullScreen :
                 winrt::Windows::UI::ViewManagement::ApplicationViewWindowingMode::Auto);

  m_window.Activate();

  const auto di = winrt::Windows::Graphics::Display::DisplayInformation::GetForCurrentView();
  di.OrientationChanged({this, &UWPHostInterface::OnOrientationChanged});

  if (di.ResolutionScale() == winrt::Windows::Graphics::Display::ResolutionScale::Scale400Percent)
  {
    screenScale = 400;
  }
  else if (di.ResolutionScale() == winrt::Windows::Graphics::Display::ResolutionScale::Scale450Percent)
  {
    screenScale = 450;
  }
  else if (di.ResolutionScale() == winrt::Windows::Graphics::Display::ResolutionScale::Scale500Percent)
  {
    screenScale = 500;
  }

  if (g_settings.forceLandscape)
  {
    di.AutoRotationPreferences(winrt::Windows::Graphics::Display::DisplayOrientations::Landscape);
  }

  currentOrientation = di.CurrentOrientation();

  if (currentOrientation == winrt::Windows::Graphics::Display::DisplayOrientations::Landscape ||
      currentOrientation == winrt::Windows::Graphics::Display::DisplayOrientations::LandscapeFlipped)
  {
    isLandscape = true;
  }
  else
  {
    isLandscape = false;
  }

  const s32 resolution_scale = static_cast<s32>(di.ResolutionScale());

  WindowInfo wi;
  wi.type = WindowInfo::Type::WinRT;
  wi.window_handle = winrt::get_unknown(m_window);
  wi.surface_scale = static_cast<float>(resolution_scale) / 100.0f;
  wi.surface_width = static_cast<u32>(m_window.Bounds().Width * wi.surface_scale);
  wi.surface_height = static_cast<s32>(m_window.Bounds().Height * wi.surface_scale);

  m_display = std::make_unique<FrontendCommon::D3D11HostDisplay>();

  if (!m_display->CreateRenderDevice(wi, g_settings.gpu_adapter, g_settings.gpu_use_debug_device,
                                     g_settings.gpu_threaded_presentation) ||
      !m_display->InitializeRenderDevice(GetShaderCacheBasePath(), g_settings.gpu_use_debug_device,
                                         g_settings.gpu_threaded_presentation) ||
      !CreateHostDisplayResources())
  {
    m_display->DestroyRenderDevice();
    m_display.reset();
    ReportError("Failed to create/initialize display render device");
    return false;
  }

  if (!CreateHostDisplayResources())
    Log_WarningPrint("Failed to create host display resources");

  Log_InfoPrintf("Host display initialized at %ux%u resolution", m_display->GetWindowWidth(),
                 m_display->GetWindowHeight());

  // system
  winrt::Windows::UI::Core::SystemNavigationManager sysNavManager =
    winrt::Windows::UI::Core::SystemNavigationManager::GetForCurrentView();
  sysNavManager.BackRequested({this, &UWPHostInterface::OnBackRequested});

  if (isLevel93)
  {
    if (g_settings.d3dnotice)
    {
      s_compatibility_window_open = true;
    }
  }
  return true;
}

void UWPHostInterface::DestroyDisplay()
{
  ReleaseHostDisplayResources();

  if (m_display)
    m_display->DestroyRenderDevice();

  m_display.reset();
}

bool UWPHostInterface::AcquireHostDisplay()
{
  return true;
}

void UWPHostInterface::ReleaseHostDisplay()
{
  // restore vsync, since we don't want to burn cycles at the menu
  m_display->SetVSync(true);
}

void UWPHostInterface::PollAndUpdate()
{
  CommonHostInterface::PollAndUpdate();

  ImGuiIO& io = ImGui::GetIO();
  if (m_text_input_requested != io.WantTextInput)
  {
    const bool activate = io.WantTextInput;
    Log_InfoPrintf("%s input pane...", activate ? "showing" : "hiding");

    m_text_input_requested = activate;
    m_dispatcher.RunAsync(winrt::Windows::UI::Core::CoreDispatcherPriority::Normal, [this, activate]() {
      const auto input_pane = winrt::Windows::UI::ViewManagement::InputPane::GetForCurrentView();
      if (input_pane)
      {
        if (activate)
          input_pane.TryShow();
        else
          input_pane.TryHide();
      }
    });
  }
}

void UWPHostInterface::RequestExit()
{
  m_shutdown_flag.store(true);
  m_dispatcher.RunAsync(winrt::Windows::UI::Core::CoreDispatcherPriority::Normal,
                        [this]() { winrt::Windows::ApplicationModel::Core::CoreApplication::Exit(); });
}

void UWPHostInterface::SetMouseMode(bool relative, bool hide_cursor) {}

void UWPHostInterface::Run()
{
  while (!appReady)
  {
    CoreWindow::GetForCurrentThread().Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
  }

  m_emulation_thread = std::thread(&UWPHostInterface::EmulationThreadEntryPoint, this);

  m_dispatcher.ProcessEvents(winrt::Windows::UI::Core::CoreProcessEventsOption::ProcessUntilQuit);
  m_shutdown_flag.store(true);
  m_emulation_thread.join();
}

LARGE_INTEGER frequency; // ticks per second
LARGE_INTEGER t1, t2;    // ticks
float elapsedTime;
double accumulatedTime = 0.0;

void UWPHostInterface::EmulationThreadEntryPoint()
{
  targetUpdateTime = g_settings.display_max_fps;
  if (targetUpdateTime < 5.0f)
  {
    targetUpdateTime = 60.0f;
  }
  targetFrameTime = 1.0 / (double)targetUpdateTime;
  // QueryPerformanceFrequency(&frequency);
  //  Get the current time at the start of the loop
  // QueryPerformanceCounter(&t1);

  if (m_fullscreen_ui_enabled)
  {
    FullscreenUI::SetDebugMenuAllowed(true);
    FullscreenUI::QueueGameListRefresh();
  }

  // process events to pick up controllers before updating input map
  PollAndUpdate();
  UpdateInputMap();

  if (m_was_running_on_suspend && ShouldSaveResumeState())
    ResumeSystemFromMostRecentState();

  auto lastTime = std::chrono::high_resolution_clock::now();

  while (!m_shutdown_flag.load() && !appClosed)
  {
    if (m_windowVisible)
    {
      /* QueryPerformanceCounter(&t2);
       elapsedTime = (t2.QuadPart - t1.QuadPart) * 1.0 / frequency.QuadPart;
       accumulatedTime += elapsedTime;*/
      auto currentTime = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> elapsedTime = currentTime - lastTime;

      RunCallbacks();
      PollAndUpdate();
      PollGamingInput();

      if (elapsedTime.count() >= targetFrameTime)
      {
        ImGui::NewFrame();

        if (System::IsRunning())
        {
          if (m_display_all_frames)
            System::RunFrame();
          else
            System::RunFrames();

          UpdateControllerMetaState();
          if (m_frame_step_request)
          {
            m_frame_step_request = false;
            PauseSystem(true);
          }
        }

        // rendering
        {
          DrawImGuiWindows();

          RenderTouchControls();

          ImGui::Render();

          ImGui::EndFrame();

          m_display->Render();

          if (System::IsRunning())
          {
            System::UpdatePerformanceCounters();

            if (m_throttler_enabled)
              System::Throttle();
          }
        }

        /*accumulatedTime -= 1.0 / 30.0;
        t1 = t2;*/
        lastTime = currentTime;
      }
      else
      {
        if (System::IsRunning())
        {
          System::RunFrame();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    }
  }

  // Save state on exit so it can be resumed
  if (!System::IsShutdown())
    PowerOffSystem(ShouldSaveResumeState());
}

void UWPHostInterface::ReportMessage(const char* message)
{
  Log_InfoPrint(message);
  AddOSDMessage(message, 10.0f);
}

void UWPHostInterface::ReportError(const char* message)
{
  Log_ErrorPrint(message);

  if (!m_display)
    return;

  const bool was_in_frame = GImGui->FrameCount != GImGui->FrameCountEnded;
  if (was_in_frame)
    ImGui::EndFrame();

  bool done = false;
  while (!done)
  {
    RunCallbacks();
    PollAndUpdate();
    if (m_fullscreen_ui_enabled)
      FullscreenUI::SetImGuiNavInputs();

    ImGui::NewFrame();
    done = FullscreenUI::DrawErrorWindow(message);
    ImGui::EndFrame();
    m_display->Render();
  }

  if (was_in_frame)
    ImGui::NewFrame();
}

bool UWPHostInterface::ConfirmMessage(const char* message)
{
  Log_InfoPrintf("Confirm: %s", message);

  if (!m_display)
    return true;

  const bool was_in_frame = GImGui->FrameCount != GImGui->FrameCountEnded;
  if (was_in_frame)
    ImGui::EndFrame();

  bool done = false;
  bool result = true;
  while (!done)
  {
    RunCallbacks();
    PollAndUpdate();
    if (m_fullscreen_ui_enabled)
      FullscreenUI::SetImGuiNavInputs();

    ImGui::NewFrame();
    done = FullscreenUI::DrawConfirmWindow(message, &result);
    ImGui::EndFrame();
    m_display->Render();
  }

  if (was_in_frame)
    ImGui::NewFrame();

  return result;
}

// Multi touch
#define MAX_TOUCH 16
struct input_pointer
{
  int id;
  float x;
  float y;
  bool isInContact;
};

std::list<input_pointer> pointers{};
input_pointer getTouch(unsigned index)
{
  input_pointer pointer{};
  if (index < pointers.size())
  {
    auto l_front = pointers.begin();
    std::advance(l_front, index);
  }
  return pointer;
}

void addOrUpdatePointer(input_pointer pointer)
{
  for (auto pItem = pointers.begin(), end = pointers.end(); pItem != end; ++pItem)
  {
    if (pItem->id == pointer.id)
    {
      pItem->x = pointer.x;
      pItem->y = pointer.y;
      pItem->isInContact = pointer.isInContact;
      return;
    }
  }

  pointers.push_back(pointer);
}

void removePointer(int id)
{
  pointers.erase(std::remove_if(pointers.begin(), pointers.end(), [&](const input_pointer& x) { return x.id == id; }),
                 pointers.end());
}

// PSX touch controller layout
// Button positions for PSX touch controller layout
float leftFixerX = 10.0f;
float righFixerX = 10.0f;

std::list<UWPHostInterface::TouchButton> touchButtons;

ImVec2 dpadSize;
ImVec2 startSelectSize;
ImVec2 lrSize;
ImVec2 lr3Size;
ImVec2 dpadUpPos;
ImVec2 dpadDownPos;
ImVec2 dpadLeftPos;
ImVec2 dpadRightPos;
ImVec2 triangPos;
ImVec2 circlePos;
ImVec2 crossPos;
ImVec2 squarePos;
ImVec2 lButtonPos;
ImVec2 rButtonPos;
ImVec2 l2ButtonPos;
ImVec2 r2ButtonPos;
ImVec2 l3ButtonPos;
ImVec2 r3ButtonPos;
ImVec2 startPos;
ImVec2 selectPos;
ImVec2 menuPos;

bool TouchActive = false;
ImVec2 joystickCenter;
float joystickRadius = 50.0f;
ImVec2 joystickPosition;
float scaleFactor = 0.0f;

void UWPHostInterface::OnOrientationChanged(winrt::Windows::Graphics::Display::DisplayInformation sender,
                                            const winrt::Windows::Foundation::IInspectable& args)
{
  currentOrientation = sender.CurrentOrientation();
  if (currentOrientation == winrt::Windows::Graphics::Display::DisplayOrientations::Landscape ||
      currentOrientation == winrt::Windows::Graphics::Display::DisplayOrientations::LandscapeFlipped)
  {
    isLandscape = true;
  }
  else
  {
    isLandscape = false;
  }
  auto size = winrt::Windows::Foundation::Size(m_window.Bounds().Width, m_window.Bounds().Height);
  const float width = size.Width;
  const float height = size.Height;
  RunLater([this, width, height]() {
    if (!m_display)
      return;
    // isResizing = true;
    m_display->ResizeRenderWindow(static_cast<s32>(width * m_display->GetWindowScale()),
                                  static_cast<s32>(height * m_display->GetWindowScale()));

    OnHostDisplayResized();

    // isResizing = false;
  });
}
void UWPHostInterface::RenderTouchControls()
{
  if (!g_settings.touch_enable)
  {
    return;
  }
  if ((!TouchActive && !g_settings.touch_force) || System::IsShutdown() || System::IsPaused() || s_quick_menu_was_open)
  {
    return;
  }
  if (!touchButtons.empty())
  {
    UpdateTouchButtonsState();
  }

  float screenWidth = ImGui::GetIO().DisplaySize.x;
  float screenHeight = ImGui::GetIO().DisplaySize.y;

  // Calculate scaling factor based on the screen width
  scaleFactor = screenWidth / 800.0f; // Assuming the original design is for a width of 800
  scaleFactor = (scaleFactor * g_settings.touch_scale);
  float centerButtonsWidthFix = 50.0f;
  float centerButtonsXFix = 55.0f;
  float analogLeftFix = 0.0f;
  float analogTopFix = 0.0f;
  float lrTopFix2 = (140.0f * g_settings.touch_scale) * scaleFactor;

  switch (currentOrientation)
  {
    case winrt::Windows::Graphics::Display::DisplayOrientations::PortraitFlipped:
    case winrt::Windows::Graphics::Display::DisplayOrientations::Portrait:
    {
      scaleFactor = screenWidth / 400.0f;
      scaleFactor = (scaleFactor * g_settings.touch_scalep);
      centerButtonsWidthFix = 40.0f;
      centerButtonsXFix = 45.0f;
      leftFixerX = 5.0f;
      righFixerX = 5.0f;

      if (!g_settings.touch_hide_dpad)
      {
        analogLeftFix = 70.0f;
        analogTopFix = 40.0f;
      }

      lrTopFix2 = -120.0f * scaleFactor;
    }
    break;
  }

  float leftFixer = leftFixerX * scaleFactor;
  float righFixer = righFixerX * scaleFactor;

  dpadSize = ImVec2(50.0f * scaleFactor, 50.0f * scaleFactor);
  startSelectSize = ImVec2(centerButtonsWidthFix * scaleFactor, 30.0f * scaleFactor);
  lrSize = ImVec2(50.0f * scaleFactor, 35.0f * scaleFactor);
  lr3Size = ImVec2(25.0f * scaleFactor, 25.0f * scaleFactor);

  // Calculate adjusted positions and sizes
  dpadUpPos = ImVec2(leftFixer + 60.0f * scaleFactor, screenHeight - dpadSize.y - 110.0f * scaleFactor);
  dpadDownPos = ImVec2(leftFixer + 60.0f * scaleFactor, screenHeight - dpadSize.y - 10.0f * scaleFactor);
  dpadLeftPos = ImVec2(leftFixer + 10.0f * scaleFactor, screenHeight - dpadSize.y - 60.0f * scaleFactor);
  dpadRightPos = ImVec2(leftFixer + 110.0f * scaleFactor, screenHeight - dpadSize.y - 60.0f * scaleFactor);

  l3ButtonPos = ImVec2(leftFixer + 72.5f * scaleFactor, screenHeight - lr3Size.y - 72.0f * scaleFactor);
  if (g_settings.touch_hide_dpad)
  {
    l3ButtonPos = ImVec2(leftFixer + 190.5f * scaleFactor, screenHeight - lr3Size.y - 72.0f * scaleFactor);
  }
  // Action buttons on the right side
  triangPos = ImVec2(screenWidth - righFixer - 110.0f * scaleFactor, screenHeight - dpadSize.y - 110.0f * scaleFactor);
  circlePos = ImVec2(screenWidth - righFixer - 60.0f * scaleFactor, screenHeight - dpadSize.y - 60.0f * scaleFactor);
  crossPos = ImVec2(screenWidth - righFixer - 110.0f * scaleFactor, screenHeight - dpadSize.y - 10.0f * scaleFactor);
  squarePos = ImVec2(screenWidth - righFixer - 160.0f * scaleFactor, screenHeight - dpadSize.y - 60.0f * scaleFactor);

  r3ButtonPos = ImVec2(screenWidth - righFixer - 97.0f * scaleFactor, screenHeight - lr3Size.y - 72.0f * scaleFactor);

  // L,R
  float lrTopFixer = (screenHeight - lrTopFix2) / 2.0f;
  lButtonPos = ImVec2((leftFixer) + 10.0f * scaleFactor, lrTopFixer + 55.0f * scaleFactor);
  rButtonPos = ImVec2(screenWidth - (righFixer)-60.0f * scaleFactor, lrTopFixer + 55.0f * scaleFactor);
  l2ButtonPos = ImVec2((leftFixer) + 10.0f * scaleFactor, lrTopFixer + 15.0f * scaleFactor);
  r2ButtonPos = ImVec2(screenWidth - (righFixer)-60.0f * scaleFactor, lrTopFixer + 15.0f * scaleFactor);

  // Start, Select
  float widthFix = ((screenWidth / 2) - (startSelectSize.x / 2));
  startPos = ImVec2(widthFix + centerButtonsXFix * scaleFactor, screenHeight - dpadSize.y + 10.0f * scaleFactor);
  selectPos = ImVec2(widthFix + 0.0f * scaleFactor, screenHeight - dpadSize.y + 10.0f * scaleFactor);
  menuPos = ImVec2(widthFix - centerButtonsXFix * scaleFactor, screenHeight - dpadSize.y + 10.0f * scaleFactor);

  if (touchButtons.empty())
  {
    float buttonFix = 120.0f * scaleFactor;
    float leftXFix = -(60.0f) * scaleFactor;
    float leftYFix = -60.0f * scaleFactor;
    float rightXFix = -(60.0f) * scaleFactor;
    float rightYFix = -60.0f * scaleFactor;

    float yActiveFix = 30.0f * scaleFactor;
    float xActiveFix = 30.0f * scaleFactor;
    float pxActiveFix = 30.0f * scaleFactor;
    float pyActiveFix = 30.0f * scaleFactor;

    touchButtons = {
      // UP
      {ICON_FA_ARROW_UP, &dpadUpPos, &dpadSize, false, ButtonsX::Up, ButtonsA::Up, buttonFix, 0.0f, leftXFix, 0.0f,
       0.0f, yActiveFix * 2, 0.0f, -pyActiveFix},

      // DOWN
      {ICON_FA_ARROW_DOWN, &dpadDownPos, &dpadSize, false, ButtonsX::Down, ButtonsA::Down, buttonFix, 0.0f, leftXFix,
       0.0f, 0.0f, yActiveFix * 2, 0.0f, -pyActiveFix},

      // LEFT
      {ICON_FA_ARROW_LEFT, &dpadLeftPos, &dpadSize, false, ButtonsX::Left, ButtonsA::Left, 0.0f, buttonFix, 0.0f,
       leftYFix, xActiveFix * 2, 0.0f, -pxActiveFix, 0.0f},

      // RIGH
      {ICON_FA_ARROW_RIGHT, &dpadRightPos, &dpadSize, false, ButtonsX::Right, ButtonsA::Right, 0.0f, buttonFix, 0.0f,
       leftYFix, xActiveFix * 2, 0.0f, -pxActiveFix, 0.0f},

      // TRAIN
      {ICON_FA_EXCLAMATION_TRIANGLE, &triangPos, &dpadSize, false, ButtonsX::Triangle, ButtonsA::Triangle, buttonFix,
       0.0f, rightXFix, 0.0f, 0.0f, yActiveFix * 2, 0.0f, -pyActiveFix},

      // CROSS
      {ICON_FA_TIMES, &crossPos, &dpadSize, false, ButtonsX::Cross, ButtonsA::Cross, buttonFix, 0.0f, rightXFix, 0.0f,
       0.0f, yActiveFix * 2, 0.0f, -pyActiveFix},

      // CIRCLE
      {ICON_FA_CIRCLE_NOTCH, &circlePos, &dpadSize, false, ButtonsX::Circle, ButtonsA::Circle, 0.0f, buttonFix, 0.0f,
       rightYFix, xActiveFix * 2, 0.0f, -pxActiveFix, 0.0f},

      // SQUARE
      {ICON_FA_SQUARE, &squarePos, &dpadSize, false, ButtonsX::Square, ButtonsA::Square, 0.0f, buttonFix, 0.0f,
       rightYFix, xActiveFix * 2, 0.0f, -pxActiveFix, 0.0f},

      {"L", &lButtonPos, &lrSize, false, ButtonsX::L1, ButtonsA::L1, 0.0f, 0.0f, 0.0f, 0.0f, xActiveFix * 2, yActiveFix,
       -pxActiveFix, 0.0f},
      {"R", &rButtonPos, &lrSize, false, ButtonsX::R1, ButtonsA::R1, 0.0f, 0.0f, 0.0f, 0.0f, xActiveFix * 2, yActiveFix,
       -pxActiveFix, 0.0f},
      {"L2", &l2ButtonPos, &lrSize, false, ButtonsX::L2, ButtonsA::L2, 0.0f, 0.0f, 0.0f, 0.0f, xActiveFix * 2,
       yActiveFix, -pxActiveFix, -pyActiveFix},

      {"R2", &r2ButtonPos, &lrSize, false, ButtonsX::R2, ButtonsA::R2, 0.0f, 0.0f, 0.0f, 0.0f, xActiveFix * 2,
       yActiveFix, -pxActiveFix, -pyActiveFix},

      {"L3", &l3ButtonPos, &lr3Size, false, ButtonsX::L3, ButtonsA::L3},
      {"R3", &r3ButtonPos, &lr3Size, false, ButtonsX::R3, ButtonsA::R3},

      {ICON_FA_PLAY, &startPos, &startSelectSize, false, ButtonsX::Start, ButtonsA::Start},
      {ICON_FA_STOP, &selectPos, &startSelectSize, false, ButtonsX::Select, ButtonsA::Select},
      {ICON_FA_BARS, &menuPos, &startSelectSize, false, ButtonsX::Count, ButtonsA::Count},
    };
  }

  ImGuiWindowFlags window_flags = 0;
  window_flags |= ImGuiWindowFlags_NoBackground;
  window_flags |= ImGuiWindowFlags_NoTitleBar;
  window_flags |= ImGuiWindowFlags_NoResize;
  window_flags |= ImGuiWindowFlags_NoInputs;
  window_flags |= ImGuiWindowFlags_NoMove;
  window_flags |= ImGuiWindowFlags_NoNavFocus;
  window_flags |= ImGuiWindowFlags_NoScrollbar;

  bool open_ptr = true;
  // Set window position to top-left corner
  ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));

  // Set window size to cover the whole screen
  ImGui::SetNextWindowSize(ImVec2(screenWidth, screenHeight));
  ImGui::Begin("PSX Touch Controller", &open_ptr, window_flags);

  for (auto pItem = touchButtons.begin(), end = touchButtons.end(); pItem != end; ++pItem)
  {
    if (pItem->trigger == ButtonsX::L3 || pItem->trigger == ButtonsX::R3)
    {
      if (!g_settings.touch_analog || !g_settings.touch_show_lr3)
      {
        continue;
      }
    }

    // DPad range
    if ((int)pItem->trigger > 3 && (int)pItem->trigger < 8)
    {
      if (g_settings.touch_hide_dpad && g_settings.touch_analog)
      {
        continue;
      }
    }

    ImVec2 position(pItem->position->x, pItem->position->y);
    ImVec2 size(pItem->size->x, pItem->size->y);
    ImGui::SetCursorPos(position);
    if (g_settings.touch_analog)
    {
      TriggerButton((int)pItem->triggera, pItem->state);
    }
    else
    {
      TriggerButton((int)pItem->trigger, pItem->state);
    }
    ImGui::Button(pItem->title, size);
  }

  if (g_settings.touch_analog)
  {
    float subCircleSize = 20.0f * scaleFactor;
    joystickRadius = 50.0f * scaleFactor;
    joystickCenter = ImVec2(leftFixer + (200.0f - analogLeftFix) * scaleFactor,
                            screenHeight - joystickRadius - (130.0f + analogTopFix) * scaleFactor);

    if (g_settings.touch_hide_dpad)
    {
      // Fix the position
      joystickRadius = (50.0f * g_settings.touch_analog_sc) * scaleFactor;
      subCircleSize = (20.0f * g_settings.touch_analog_sc) * scaleFactor;
      joystickCenter = ImVec2(leftFixer + (90.0f - analogLeftFix) * scaleFactor,
                              screenHeight - joystickRadius - (20.0f + analogTopFix) * scaleFactor);
    }
    // Render the joystick using ImDrawList
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddCircleFilled(joystickCenter, joystickRadius, IM_COL32(128, 128, 128, (80 + g_settings.touch_opacity)));
    drawList->AddCircleFilled(ImVec2(joystickCenter.x + joystickPosition.x * joystickRadius,
                                     joystickCenter.y + joystickPosition.y * joystickRadius),
                              subCircleSize, IM_COL32(155, 125, 115, (100 + g_settings.touch_opacity)));
  }

  ImGui::End();
}

struct AnalogAxisValues
{
  float xAxis = 0.0f;
  float yAxis = 0.0f;
};
AnalogAxisValues analogAxes;

int tempPort = 1;
void UWPHostInterface::TriggerButton(int code, bool pressed)
{
  bool isActive = false;
  if (code == (int)ButtonsX::Left)
  {
    isActive = leftState;
  }
  else if (code == (int)ButtonsX::Right)
  {
    isActive = rightState;
  }
  else if (code == (int)ButtonsX::Up)
  {
    isActive = upState;
  }
  else if (code == (int)ButtonsX::Down)
  {
    isActive = downState;
  }

  if (pressed || isActive)
  {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.2f, 0.6f, 0.8f});
  }
  else
  {
    float defaultOpacity = g_settings.touch_opacity * 0.01f;
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.3f, 0.4f, defaultOpacity});
  }
  if (code == (int)ButtonsX::Count || code == (int)ButtonsA::Count)
  {
    if (pressed)
    {
      if (!s_quick_menu_was_open)
      {
        FullscreenUI::OpenQuickMenu();
        // In case something stuck, reset when quick menu called
        pointers.clear();
      }
    }
  }
  else
  {
    Controller* controller = System::GetController((g_settings.touch_port - 1));
    if (controller)
    {
      controller->SetButtonState((s32)code, pressed);

      if (g_settings.touch_analog)
      {
        controller->SetAxisState((s32)Axis::LeftX, analogAxes.xAxis);
        controller->SetAxisState((s32)Axis::LeftY, analogAxes.yAxis);
      }
    }
    if (tempPort != g_settings.touch_port)
    {
      Controller* controller = System::GetController((tempPort - 1));
      if (controller)
      {
        for (u32 i = 1; i < 16; i++)
        {
          // reset all active controls on old port
          controller->SetButtonState(i, false);
        }
      }
      tempPort = g_settings.touch_port;
    }
  }
}

void UWPHostInterface::TriggerButton(int port, int code, bool pressed, bool analog, Axis type, float value)
{
  Controller* controller = System::GetController(port);
  if (controller)
  {
    if (analog)
    {
      controller->SetAxisState((s32)type, value);
    }
    else
    {
      controller->SetButtonState((s32)code, pressed);
    }
  }
}

bool UWPHostInterface::TriggerHook(int port, int code, bool pressed, bool analog, Axis type, float value)
{
  return FullscreenUI::HandleKeyboardBinding(code, pressed);
}

bool IsPointInButton(const ImVec2& point, const ImVec2& buttonPos, const ImVec2& buttonSize)
{
  return (point.x >= buttonPos.x && point.x <= buttonPos.x + buttonSize.x && point.y >= buttonPos.y &&
          point.y <= buttonPos.y + buttonSize.y);
}

bool IsPointInCircle(const ImVec2& point, const ImVec2& center, float radius)
{
  float distance = sqrt((point.x - center.x) * (point.x - center.x) + (point.y - center.y) * (point.y - center.y));
  return distance <= radius;
}

void UWPHostInterface::UpdateTouchButtonsState()
{
  if (!pointers.empty())
  {
    for (auto pItem = touchButtons.begin(), end = touchButtons.end(); pItem != end; ++pItem)
    {
      for (auto sItem = pointers.begin(); sItem != pointers.end(); ++sItem)
      {
        // Get the touch point's coordinates
        float touchX = sItem->x;
        float touchY = sItem->y;

        if (g_settings.touch_log)
        {
          Log_WarningPrintf("Pointer: id->%d, x->%.2f, y->%.2f, contact->%s", sItem->id, sItem->x, sItem->y,
                            (sItem->isInContact ? "true" : "false"));
        }
        // Translate touch point's coordinates to ImGui window space
        ImVec2 touchPos = ImVec2(touchX, touchY);

        float extendedPX = pItem->px_extended;
        float extendedPY = pItem->py_extended;
        float extendedX = pItem->x_extended;
        float extendedY = pItem->y_extended;

        if ((int)pItem->trigger > 11 && (int)pItem->trigger < 16)
        {
          if ((!g_settings.touch_action_extra && !pItem->state) || !isLandscape)
          {
            extendedPX = 0.0f;
            extendedPY = 0.0f;
            extendedX = 0.0f;
            extendedY = 0.0f;
          }
        }

        if ((int)pItem->trigger > 3 && (int)pItem->trigger < 8)
        {
          if ((!g_settings.touch_dpad_extra && !pItem->state) || !isLandscape)
          {
            bool ignore = false;
            if (g_settings.touch_dpad_extra)
            {
              if (pItem->trigger == ButtonsX::Up || pItem->trigger == ButtonsX::Left ||
                  pItem->trigger == ButtonsX::Right)
              {
                ignore = true;
                if (pItem->trigger == ButtonsX::Left || pItem->trigger == ButtonsX::Right)
                {
                  extendedPX /= 2.0f;
                  extendedPY /= 2.0f;
                  extendedX /= 2.0f;
                  extendedY /= 2.0f;
                }
              }
            }
            if (!ignore)
            {
              extendedPX = 0.0f;
              extendedPY = 0.0f;
              extendedX = 0.0f;
              extendedY = 0.0f;
            }
          }
        }

        ImVec2 position(pItem->position->x + extendedPX, pItem->position->y + extendedPY);
        ImVec2 size(pItem->size->x + extendedX, pItem->size->y + extendedY);

        // Check if touch point is within the area of a button
        bool buttonState = IsPointInButton(touchPos, position, size);

        if (!buttonState && pItem->state)
        {
          // If button was active extend it's space for more accuracy
          float extendedX_a = pItem->x_extended_a;
          float extendedY_a = pItem->y_extended_a;
          float extendedPX_a = pItem->px_extended_a;
          float extendedPY_a = pItem->py_extended_a;

          ImVec2 position_a(pItem->position->x + extendedPX_a, pItem->position->y + extendedPY_a);
          ImVec2 size_a(pItem->size->x + extendedX_a, pItem->size->y + extendedY_a);

          buttonState = IsPointInButton(touchPos, position_a, size_a);
        }

        pItem->state = buttonState;
        if (g_settings.touch_log)
        {
          Log_WarningPrintf("Button (%s): %s", pItem->title, (pItem->state ? "true" : "false"));
        }
        if (pItem->state)
        {
          break;
        }
      }
    }
  }
  else
  {
    for (auto pItem = touchButtons.begin(), end = touchButtons.end(); pItem != end; ++pItem)
    {
      pItem->state = false;
    }
  }
  if (g_settings.touch_analog)
  {
    // Analog
    joystickPosition = ImVec2(0.0f, 0.0f);
    for (auto sItem = pointers.begin(); sItem != pointers.end(); ++sItem)
    {
      // Get the touch point's coordinates
      float touchX = sItem->x;
      float touchY = sItem->y;

      // Translate touch point's coordinates to ImGui window space
      ImVec2 touchPos = ImVec2(touchX, touchY);
      float analogFix1 = 100.0f;
      float analogFix2 = 50.0f;
      if (g_settings.touch_hide_dpad)
      {
        analogFix1 = analogFix1 * g_settings.touch_analog_sc;
        analogFix2 = analogFix2 * g_settings.touch_analog_sc;
      }
      ImVec2 size = ImVec2(joystickRadius + analogFix1 * scaleFactor, joystickRadius + analogFix1 * scaleFactor);

      ImVec2 position =
        ImVec2(joystickCenter.x - analogFix2 * scaleFactor, joystickCenter.y - analogFix2 * scaleFactor);

      bool isActive = IsPointInButton(touchPos, position, size);
      if (isActive)
      {
        if (g_settings.touch_log)
        {
          Log_WarningPrintf("Analog: id->%d, x->%.2f, y->%.2f, contact->%s", sItem->id, sItem->x, sItem->y,
                            (sItem->isInContact ? "true" : "false"));
          Log_WarningPrintf("Analog (position): x->%.2f, y->%.2f", position.x, position.y);
        }

        // Calculate the joystick position based on touch input
        if (IsPointInCircle(touchPos, joystickCenter, joystickRadius * 2.0f))
        {
          // Calculate the offset from the center of the joystick
          ImVec2 offset = ImVec2(touchPos.x - joystickCenter.x, touchPos.y - joystickCenter.y);

          // Limit the offset to the joystick radius
          float length = sqrt(offset.x * offset.x + offset.y * offset.y);
          if (length > joystickRadius)
          {
            offset.x *= joystickRadius / length;
            offset.y *= joystickRadius / length;
          }

          joystickPosition = ImVec2(offset.x / joystickRadius, offset.y / joystickRadius);
        }
        break;
      }
    }
    analogAxes.xAxis = joystickPosition.x * (scaleFactor * g_settings.touch_analog_s);
    analogAxes.yAxis = joystickPosition.y * (scaleFactor * g_settings.touch_analog_s);
  }
}

void UWPHostInterface::RunLater(std::function<void()> callback)
{
  std::unique_lock<std::mutex> lock(m_queued_callbacks_lock);
  m_queued_callbacks.push_back(std::move(callback));
}

void UWPHostInterface::OnDisplayInvalidated() {}

void UWPHostInterface::OnSystemPerformanceCountersUpdated() {}

bool UWPHostInterface::IsFullscreen() const
{
  return m_appview.IsFullScreenMode();
}

bool UWPHostInterface::SetFullscreen(bool enabled)
{
  m_dispatcher.RunAsync(winrt::Windows::UI::Core::CoreDispatcherPriority::Normal, [this, enabled]() {
    if (enabled)
      m_appview.TryEnterFullScreenMode();
    else
      m_appview.ExitFullScreenMode();
  });

  return true;
}

void UWPHostInterface::RunCallbacks()
{
  std::unique_lock<std::mutex> lock(m_queued_callbacks_lock);

  while (!m_queued_callbacks.empty())
  {
    auto callback = std::move(m_queued_callbacks.front());
    m_queued_callbacks.pop_front();
    lock.unlock();
    callback();
    lock.lock();
  }
}

void UWPHostInterface::SetWindow(const winrt::Windows::UI::Core::CoreWindow& window)
{
  m_window = window;
  m_dispatcher = m_window.Dispatcher();

  window.Closed({this, &UWPHostInterface::OnClosed});
  window.SizeChanged({this, &UWPHostInterface::OnSizeChanged});
  window.KeyDown({this, &UWPHostInterface::OnKeyDown});
  window.KeyUp({this, &UWPHostInterface::OnKeyUp});
  window.CharacterReceived({this, &UWPHostInterface::OnCharacterReceived});
  window.PointerPressed({this, &UWPHostInterface::OnPointerPressed});
  window.PointerReleased({this, &UWPHostInterface::OnPointerReleased});
  window.PointerMoved({this, &UWPHostInterface::OnPointerMoved});
  window.PointerWheelChanged({this, &UWPHostInterface::OnPointerWheelChanged});
  window.VisibilityChanged({this, &UWPHostInterface::OnVisibilityChanged});

  Initialize();
}

void UWPHostInterface::OnActivated(const IInspectable&,
                                   const winrt::Windows::ApplicationModel::Activation::IActivatedEventArgs& args)
{
  DetectLaunchItem(args);
}

void UWPHostInterface::OnVisibilityChanged(const IInspectable&,
                                           const winrt::Windows::UI::Core::VisibilityChangedEventArgs& args)
{
  m_windowVisible = args.Visible();
}

winrt::Windows::Storage::StorageFolder dataFolder = nullptr;
extern std::list<std::string> GetFutureAccessList();
extern winrt::hstring GetDataFromLocalSettings(winrt::hstring key);
bool UWPHostInterface::SetDirectories()
{
  const auto install_location = winrt::Windows::ApplicationModel::Package::Current().InstalledLocation();
  m_program_directory = StringUtil::WideStringToUTF8String(install_location.Path());
  if (m_program_directory.empty())
  {
    Log_ErrorPrintf("Failed to get install location");
    return false;
  }

  Log_InfoPrintf("Program directory: %s", m_program_directory.c_str());

  dataFolder = winrt::Windows::Storage::ApplicationData::Current().LocalFolder();

  auto customData = GetDataFromLocalSettings(L"duckData");
  if (!customData.empty())
  {
    try
    {
      if (customData != dataFolder.Path())
      {
        auto testFolder = winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(customData).get();
        if (testFolder != nullptr)
        {
          auto folderName = testFolder.Name();
          if (folderName != L"DuckStation")
          {
            dataFolder =
              testFolder
                .CreateFolderAsync(L"DuckStation", winrt::Windows::Storage::CreationCollisionOption::OpenIfExists)
                .get();
          }
          else
          {
            dataFolder = testFolder;
          }
        }
      }
    }
    catch (...)
    {
    }
  }

  m_user_directory = StringUtil::WideStringToUTF8String(dataFolder.Path());
  if (m_user_directory.empty())
  {
    Log_ErrorPrintf("Failed to get user directory");
    return false;
  }

  Log_InfoPrintf("User directory: %s", m_user_directory.c_str());

  GetFutureAccessList();
  return true;
}

void UWPHostInterface::OnSuspending(const IInspectable&,
                                    const winrt::Windows::ApplicationModel::SuspendingEventArgs& args)
{
  // appSuspended = true;
  if (IsEmulationThreadRunning())
  {
    winrt::Windows::ApplicationModel::SuspendingDeferral deferral = args.SuspendingOperation().GetDeferral();

    Concurrency::create_task([this, deferral]() {
      m_was_running_on_suspend.store(System::IsRunning());
      // PauseSystem(true);
      // m_suspend_sync_event.Signal();
      m_display->Trim();
      deferral.Complete();
    });

    // m_suspend_sync_event.Wait();
  }
  else
  {
    winrt::Windows::ApplicationModel::SuspendingDeferral deferral = args.SuspendingOperation().GetDeferral();
    Concurrency::create_task([this, deferral]() {
      m_display->Trim();
      deferral.Complete();
    });
  }
}

void UWPHostInterface::OnResuming(const IInspectable&, const IInspectable&)
{
  // appSuspended = false;
  if (IsEmulationThreadRunning())
  {
    if (m_was_running_on_suspend.load())
      RunLater([this]() { PauseSystem(false); });
  }
  else
  {
    RunLater([this]() {
      if (ShouldSaveResumeState())
        ResumeSystemFromMostRecentState();
    });
  }
}

void UWPHostInterface::OnClosed(const IInspectable&, const winrt::Windows::UI::Core::CoreWindowEventArgs& args)
{
  // appSuspended = true;
  appClosed = true;
  try
  {
    FullscreenUI::DoPowerOff();
  }
  catch (...)
  {
  }
  m_display->DestroyRenderDevice();
  if (IsEmulationThreadRunning())
  {
    m_shutdown_flag.store(true);
    m_emulation_thread.join();
  }
  args.Handled(true);
}

void UWPHostInterface::OnSizeChanged(const IInspectable&,
                                     const winrt::Windows::UI::Core::WindowSizeChangedEventArgs& args)
{
  if (IsEmulationThreadRunning())
  {
    const auto size = args.Size();
    const float width = size.Width;
    const float height = size.Height;

    // const auto di = winrt::Windows::Graphics::Display::DisplayInformation::GetForCurrentView();
    // di.OrientationChanged({this, &UWPHostInterface::OnOrientationChanged});
    // currentOrientation = di.CurrentOrientation();

    // if (currentOrientation == winrt::Windows::Graphics::Display::DisplayOrientations::Landscape ||
    //     currentOrientation == winrt::Windows::Graphics::Display::DisplayOrientations::LandscapeFlipped)
    //{
    //   isLandscape = true;
    // }
    // else
    //{
    //   isLandscape = false;
    // }
    RunLater([this, width, height]() {
      if (!m_display)
        return;
      // isResizing = true;
      m_display->ResizeRenderWindow(static_cast<s32>(width * m_display->GetWindowScale()),
                                    static_cast<s32>(height * m_display->GetWindowScale()));

      OnHostDisplayResized();

      // isResizing = false;
    });
  }
  // args.Handled(true);
}

bool inputReset = false;
bool inputReset2 = false;
void UWPHostInterface::updateButtonState(winrt::Windows::System::VirtualKey targetKey, bool buttonState)
{
  const HostKeyCode code = static_cast<HostKeyCode>(targetKey);
  RunLater([this, code, buttonState]() {
    ImGuiIO& io = ImGui::GetIO();
    if (buttonState)
    {
      if (code < countof(io.KeysDown))
        io.KeysDown[code] = true;
    }
    else
    {
      if (code < countof(io.KeysDown))
        io.KeysDown[code] = false;
    }
    if (!io.WantCaptureKeyboard)
      HandleHostKeyEvent(code, 0, true);
  });
}

void UWPHostInterface::PollGamingInput()
{
  if (g_settings.gaming_event && !input_changed)
  {
    if (!myGamepads.empty())
    {
      int portIndex = 0;
      for (auto gItem = myGamepads.begin(); gItem != myGamepads.end(); ++gItem)
      {
        GamepadReading reading = gItem->GetCurrentReading();
        if (System::IsShutdown() || System::IsPaused() || s_quick_menu_was_open)
        {
          if (g_settings.gaming_ui)
          {
            if (reading.Buttons != GamepadButtons::None || !inputReset2)
            {
              inputReset2 = reading.Buttons == GamepadButtons::None;
              if (UpButton.direct == -1 || UpButton.direct == -2)
              {
                updateButtonState(winrt::Windows::System::VirtualKey::Up,
                                  UpButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
              }
              else
              {
                updateButtonState(winrt::Windows::System::VirtualKey::Up, (int)reading.Buttons & (int)UpButton.gaming);
              }
              if (DownButton.direct == -1 || DownButton.direct == -2)
              {
                updateButtonState(winrt::Windows::System::VirtualKey::Down,
                                  DownButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
              }
              else
              {
                updateButtonState(winrt::Windows::System::VirtualKey::Down,
                                  (int)reading.Buttons & (int)DownButton.gaming);
              }
              if (RightButton.direct == -1 || RightButton.direct == -2)
              {
                updateButtonState(winrt::Windows::System::VirtualKey::Right,
                                  RightButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
              }
              else
              {
                updateButtonState(winrt::Windows::System::VirtualKey::Right,
                                  (int)reading.Buttons & (int)RightButton.gaming);
              }
              if (LeftButton.direct == -1 || LeftButton.direct == -2)
              {
                updateButtonState(winrt::Windows::System::VirtualKey::Left,
                                  LeftButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
              }
              else
              {
                updateButtonState(winrt::Windows::System::VirtualKey::Left,
                                  (int)reading.Buttons & (int)LeftButton.gaming);
              }
              if (AButton.direct == -1 || AButton.direct == -2)
              {
                updateButtonState(winrt::Windows::System::VirtualKey::Space,
                                  AButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
              }
              else
              {
                updateButtonState(winrt::Windows::System::VirtualKey::Space,
                                  (int)reading.Buttons & (int)AButton.gaming);
              }
              if (BButton.direct == -1 || BButton.direct == -2)
              {
                updateButtonState(winrt::Windows::System::VirtualKey::Escape,
                                  BButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
              }
              else
              {
                updateButtonState(winrt::Windows::System::VirtualKey::Escape,
                                  (int)reading.Buttons & (int)BButton.gaming);
              }
            }
          }
        }
        else
        {
          if (reading.Buttons != GamepadButtons::None || !inputReset)
          {
            inputReset = reading.Buttons == GamepadButtons::None;

            if (UpButton.direct == -1 || UpButton.direct == -2)
            {
              TriggerButton(portIndex, (int)ButtonsA::Up,
                            UpButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::Up, (int)reading.Buttons & (int)UpButton.gaming);
            }

            if (DownButton.direct == -1 || DownButton.direct == -2)
            {
              TriggerButton(portIndex, (int)ButtonsA::Down,
                            DownButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::Down, (int)reading.Buttons & (int)DownButton.gaming);
            }
            if (LeftButton.direct == -1 || LeftButton.direct == -2)
            {
              TriggerButton(portIndex, (int)ButtonsA::Left,
                            LeftButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::Left, (int)reading.Buttons & (int)LeftButton.gaming);
            }
            if (RightButton.direct == -1 || RightButton.direct == -2)
            {
              TriggerButton(portIndex, (int)ButtonsA::Right,
                            RightButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::Right, (int)reading.Buttons & (int)RightButton.gaming);
            }

            if (StartButton.direct == -1 || StartButton.direct == -2)
            {
              TriggerButton(portIndex, (int)ButtonsA::Start,
                            StartButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::Start, (int)reading.Buttons & (int)StartButton.gaming);
            }

            if (SelectButton.direct == -1 || SelectButton.direct == -2)
            {
              TriggerButton(portIndex, (int)ButtonsA::Select,
                            SelectButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::Select, (int)reading.Buttons & (int)SelectButton.gaming);
            }
            if (AButton.direct == -1 || AButton.direct == -2)
            {
              TriggerButton(portIndex, (int)ButtonsA::Cross,
                            AButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::Cross, (int)reading.Buttons & (int)AButton.gaming);
            }
            if (BButton.direct == -1 || BButton.direct == -2)
            {
              TriggerButton(portIndex, (int)ButtonsA::Circle,
                            BButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::Circle, (int)reading.Buttons & (int)BButton.gaming);
            }
            if (XButton.direct == -1 || XButton.direct == -2)
            {
              TriggerButton(portIndex, (int)ButtonsA::Square,
                            XButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::Square, (int)reading.Buttons & (int)XButton.gaming);
            }
            if (YButton.direct == -1 || YButton.direct == -2)
            {
              TriggerButton(portIndex, (int)ButtonsA::Triangle,
                            YButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::Triangle, (int)reading.Buttons & (int)YButton.gaming);
            }
            if (R1Button.direct == -1 || R1Button.direct == -2)
            {
              TriggerButton(portIndex, (int)ButtonsA::R1,
                            R1Button.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::R1, (int)reading.Buttons & (int)R1Button.gaming);
            }
            if (L1Button.direct == -1 || L1Button.direct == -2)
            {
              TriggerButton(portIndex, (int)ButtonsA::L1,
                            L1Button.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::L1, (int)reading.Buttons & (int)L1Button.gaming);
            }

            if (R2Button.direct != -2)
            {
              TriggerButton(portIndex, (int)ButtonsA::R2, (int)reading.Buttons & (int)R2Button.gaming);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::R2, reading.RightTrigger > 0.0);
            }

            if (L2Button.direct != -1)
            {
              TriggerButton(portIndex, (int)ButtonsA::L2, (int)reading.Buttons & (int)L2Button.gaming);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::L2, reading.LeftTrigger > 0.0);
            }

            if (R3Button.direct == -1 || R3Button.direct == -2)
            {
              TriggerButton(portIndex, (int)ButtonsA::R3,
                            R3Button.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::R3, (int)reading.Buttons & (int)R3Button.gaming);
            }
            if (L3Button.direct == -1 || L3Button.direct == -2)
            {
              TriggerButton(portIndex, (int)ButtonsA::L3,
                            L3Button.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::L3, (int)reading.Buttons & (int)L3Button.gaming);
            }

            // Read the analog axis values
            float leftThumbstickX = reading.LeftThumbstickX;
            float leftThumbstickY = reading.LeftThumbstickY;
            float rightThumbstickX = reading.RightThumbstickX;
            float rightThumbstickY = reading.RightThumbstickY;

            TriggerButton(portIndex, (int)ButtonsA::Count, true, true, Axis::LeftX, leftThumbstickX);
            TriggerButton(portIndex, (int)ButtonsA::Count, true, true, Axis::LeftY, leftThumbstickY);

            TriggerButton(portIndex, (int)ButtonsA::Count, true, true, Axis::RightX, rightThumbstickX);
            TriggerButton(portIndex, (int)ButtonsA::Count, true, true, Axis::RightY, rightThumbstickY);

            if (g_settings.gaming_quick)
            {
              if (((int)reading.Buttons & (int)StartButton.gaming) && ((int)reading.Buttons & (int)SelectButton.gaming))
              {
                if (!s_quick_menu_was_open)
                {
                  FullscreenUI::OpenQuickMenu();
                }
              }
            }
          }
        }
        portIndex++;
      }
    }
  }
}

#ifndef NO_RAW
void UWPHostInterface::PollGamingRawInput()
{
  if (g_settings.gaming_event && !input_changed)
  {
    if (!myRawGamepads.empty())
    {
      int portIndex = 0;
      for (auto gItem = myRawGamepads.begin(); gItem != myRawGamepads.end(); ++gItem)
      {
        GamepadReading reading = gItem->GetCurrentReading();
        if (System::IsShutdown() || System::IsPaused() || s_quick_menu_was_open)
        {
          //..
          if (g_settings.gaming_ui)
          {
            if (reading.Buttons != GamepadButtons::None || !inputReset2)
            {
              inputReset2 = reading.Buttons == GamepadButtons::None;
              if (UpButton.direct == -1 || UpButton.direct == -2)
              {
                updateButtonState(winrt::Windows::System::VirtualKey::Up,
                                  UpButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
              }
              else
              {
                updateButtonState(winrt::Windows::System::VirtualKey::Up, (int)reading.Buttons & (int)UpButton.gaming);
              }
              if (DownButton.direct == -1 || DownButton.direct == -2)
              {
                updateButtonState(winrt::Windows::System::VirtualKey::Down,
                                  DownButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
              }
              else
              {
                updateButtonState(winrt::Windows::System::VirtualKey::Down,
                                  (int)reading.Buttons & (int)DownButton.gaming);
              }
              if (RightButton.direct == -1 || RightButton.direct == -2)
              {
                updateButtonState(winrt::Windows::System::VirtualKey::Right,
                                  RightButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
              }
              else
              {
                updateButtonState(winrt::Windows::System::VirtualKey::Right,
                                  (int)reading.Buttons & (int)RightButton.gaming);
              }
              if (LeftButton.direct == -1 || LeftButton.direct == -2)
              {
                updateButtonState(winrt::Windows::System::VirtualKey::Left,
                                  LeftButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
              }
              else
              {
                updateButtonState(winrt::Windows::System::VirtualKey::Left,
                                  (int)reading.Buttons & (int)LeftButton.gaming);
              }
              if (AButton.direct == -1 || AButton.direct == -2)
              {
                updateButtonState(winrt::Windows::System::VirtualKey::Space,
                                  AButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
              }
              else
              {
                updateButtonState(winrt::Windows::System::VirtualKey::Space,
                                  (int)reading.Buttons & (int)AButton.gaming);
              }
              if (BButton.direct == -1 || BButton.direct == -2)
              {
                updateButtonState(winrt::Windows::System::VirtualKey::Escape,
                                  BButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
              }
              else
              {
                updateButtonState(winrt::Windows::System::VirtualKey::Escape,
                                  (int)reading.Buttons & (int)BButton.gaming);
              }
            }
          }
        }
        else
        {
          if (reading.Buttons != GamepadButtons::None || !inputReset)
          {
            inputReset = reading.Buttons == GamepadButtons::None;

            if (UpButton.direct == -1 || UpButton.direct == -2)
            {
              TriggerButton(portIndex, (int)ButtonsA::Up,
                            UpButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::Up, (int)reading.Buttons & (int)UpButton.gaming);
            }

            if (DownButton.direct == -1 || DownButton.direct == -2)
            {
              TriggerButton(portIndex, (int)ButtonsA::Down,
                            DownButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::Down, (int)reading.Buttons & (int)DownButton.gaming);
            }
            if (LeftButton.direct == -1 || LeftButton.direct == -2)
            {
              TriggerButton(portIndex, (int)ButtonsA::Left,
                            LeftButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::Left, (int)reading.Buttons & (int)LeftButton.gaming);
            }
            if (RightButton.direct == -1 || RightButton.direct == -2)
            {
              TriggerButton(portIndex, (int)ButtonsA::Right,
                            RightButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::Right, (int)reading.Buttons & (int)RightButton.gaming);
            }

            if (StartButton.direct == -1 || StartButton.direct == -2)
            {
              TriggerButton(portIndex, (int)ButtonsA::Start,
                            StartButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::Start, (int)reading.Buttons & (int)StartButton.gaming);
            }

            if (SelectButton.direct == -1 || SelectButton.direct == -2)
            {
              TriggerButton(portIndex, (int)ButtonsA::Select,
                            SelectButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::Select, (int)reading.Buttons & (int)SelectButton.gaming);
            }
            if (AButton.direct == -1 || AButton.direct == -2)
            {
              TriggerButton(portIndex, (int)ButtonsA::Cross,
                            AButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::Cross, (int)reading.Buttons & (int)AButton.gaming);
            }
            if (BButton.direct == -1 || BButton.direct == -2)
            {
              TriggerButton(portIndex, (int)ButtonsA::Circle,
                            BButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::Circle, (int)reading.Buttons & (int)BButton.gaming);
            }
            if (XButton.direct == -1 || XButton.direct == -2)
            {
              TriggerButton(portIndex, (int)ButtonsA::Square,
                            XButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::Square, (int)reading.Buttons & (int)XButton.gaming);
            }
            if (YButton.direct == -1 || YButton.direct == -2)
            {
              TriggerButton(portIndex, (int)ButtonsA::Triangle,
                            YButton.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::Triangle, (int)reading.Buttons & (int)YButton.gaming);
            }
            if (R1Button.direct == -1 || R1Button.direct == -2)
            {
              TriggerButton(portIndex, (int)ButtonsA::R1,
                            R1Button.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::R1, (int)reading.Buttons & (int)R1Button.gaming);
            }
            if (L1Button.direct == -1 || L1Button.direct == -2)
            {
              TriggerButton(portIndex, (int)ButtonsA::L1,
                            L1Button.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::L1, (int)reading.Buttons & (int)L1Button.gaming);
            }

            if (R2Button.direct != -2)
            {
              TriggerButton(portIndex, (int)ButtonsA::R2, (int)reading.Buttons & (int)R2Button.gaming);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::R2, reading.RightTrigger > 0.0);
            }

            if (L2Button.direct != -1)
            {
              TriggerButton(portIndex, (int)ButtonsA::L2, (int)reading.Buttons & (int)L2Button.gaming);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::L2, reading.LeftTrigger > 0.0);
            }

            if (R3Button.direct == -1 || R3Button.direct == -2)
            {
              TriggerButton(portIndex, (int)ButtonsA::R3,
                            R3Button.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::R3, (int)reading.Buttons & (int)R3Button.gaming);
            }
            if (L3Button.direct == -1 || L3Button.direct == -2)
            {
              TriggerButton(portIndex, (int)ButtonsA::L3,
                            L3Button.direct == -1 ? reading.LeftTrigger > 0.0 : reading.RightTrigger > 0.0);
            }
            else
            {
              TriggerButton(portIndex, (int)ButtonsA::L3, (int)reading.Buttons & (int)L3Button.gaming);
            }

            // Read the analog axis values
            float leftThumbstickX = reading.LeftThumbstickX;
            float leftThumbstickY = reading.LeftThumbstickY;
            float rightThumbstickX = reading.RightThumbstickX;
            float rightThumbstickY = reading.RightThumbstickY;

            TriggerButton(portIndex, (int)ButtonsA::Count, true, true, Axis::LeftX, leftThumbstickX);
            TriggerButton(portIndex, (int)ButtonsA::Count, true, true, Axis::LeftY, leftThumbstickY);

            TriggerButton(portIndex, (int)ButtonsA::Count, true, true, Axis::RightX, rightThumbstickX);
            TriggerButton(portIndex, (int)ButtonsA::Count, true, true, Axis::RightY, rightThumbstickY);

            if (g_settings.gaming_quick)
            {
              if (((int)reading.Buttons & (int)StartButton.gaming) && ((int)reading.Buttons & (int)SelectButton.gaming))
              {
                if (!s_quick_menu_was_open)
                {
                  FullscreenUI::OpenQuickMenu();
                }
              }
            }
          }
        }
        portIndex++;
      }
    }
  }
}
#endif

bool startState = false;
bool selectState = false;
void UWPHostInterface::NativeDPadEvent(const winrt::Windows::UI::Core::KeyEventArgs& args, bool buttonState)
{
  if (g_settings.native_event)
  {
    int port = g_settings.native_port - 1;

    auto keyCode = args.VirtualKey();
    bool isDPad = (int)keyCode >= 195 && (int)keyCode <= 218; // DPad buttons range
    if (isDPad)
    {
      if (System::IsShutdown() || System::IsPaused() || s_quick_menu_was_open)
      {
        if (g_settings.native_ui)
        {
          if (keyCode == UpButton.native)
          {
            updateButtonState(winrt::Windows::System::VirtualKey::Up, buttonState);
          }
          else if (keyCode == DownButton.native)
          {
            updateButtonState(winrt::Windows::System::VirtualKey::Down, buttonState);
          }
          else if (keyCode == LeftButton.native)
          {
            updateButtonState(winrt::Windows::System::VirtualKey::Left, buttonState);
          }
          else if (keyCode == RightButton.native)
          {
            updateButtonState(winrt::Windows::System::VirtualKey::Right, buttonState);
          }
          else if (keyCode == AButton.native)
          {
            updateButtonState(winrt::Windows::System::VirtualKey::Space, buttonState);
          }
          else if (keyCode == BButton.native)
          {
            updateButtonState(winrt::Windows::System::VirtualKey::Escape, buttonState);
          }
        }
      }
      else
      {
        if (keyCode == UpButton.native)
        {
          TriggerButton(port, (int)ButtonsA::Up, buttonState);
        }
        else if (keyCode == DownButton.native)
        {
          TriggerButton(port, (int)ButtonsA::Down, buttonState);
        }
        else if (keyCode == LeftButton.native)
        {
          TriggerButton(port, (int)ButtonsA::Left, buttonState);
        }
        else if (keyCode == RightButton.native)
        {
          TriggerButton(port, (int)ButtonsA::Right, buttonState);
        }
        else if (keyCode == StartButton.native)
        {
          TriggerButton(port, (int)ButtonsA::Start, buttonState);
          startState = buttonState;
        }
        else if (keyCode == SelectButton.native)
        {
          TriggerButton(port, (int)ButtonsA::Select, buttonState);
          selectState = buttonState;
        }
        else if (keyCode == AButton.native)
        {
          TriggerButton(port, (int)ButtonsA::Cross, buttonState);
        }
        else if (keyCode == BButton.native)
        {
          TriggerButton(port, (int)ButtonsA::Circle, buttonState);
        }
        else if (keyCode == XButton.native)
        {
          TriggerButton(port, (int)ButtonsA::Square, buttonState);
        }
        else if (keyCode == YButton.native)
        {
          TriggerButton(port, (int)ButtonsA::Triangle, buttonState);
        }
        else if (keyCode == R1Button.native)
        {
          TriggerButton(port, (int)ButtonsA::R1, buttonState);
        }
        else if (keyCode == R2Button.native)
        {
          TriggerButton(port, (int)ButtonsA::R2, buttonState);
        }
        else if (keyCode == L1Button.native)
        {
          TriggerButton(port, (int)ButtonsA::L1, buttonState);
        }
        else if (keyCode == L2Button.native)
        {
          TriggerButton(port, (int)ButtonsA::L2, buttonState);
        }
        else if (keyCode == R3Button.native)
        {
          TriggerButton(port, (int)ButtonsA::R3, buttonState);
        }
        else if (keyCode == L3Button.native)
        {
          TriggerButton(port, (int)ButtonsA::L3, buttonState);
        }

        if (g_settings.gaming_quick)
        {
          if (startState && selectState)
          {
            if (!s_quick_menu_was_open)
            {
              FullscreenUI::OpenQuickMenu();
            }
          }
        }
      }
    }
  }

  args.Handled(true);
}

void UWPHostInterface::OnKeyDown(const IInspectable&, const winrt::Windows::UI::Core::KeyEventArgs& args)
{
  if (TriggerHook(0, (int)args.VirtualKey(), true))
  {
    return;
  }
  TouchActive = false;
  const auto status = args.KeyStatus();
  if (!status.WasKeyDown && !status.IsKeyReleased && IsEmulationThreadRunning())
  {
    const HostKeyCode code = static_cast<HostKeyCode>(args.VirtualKey());
    RunLater([this, code]() {
      ImGuiIO& io = ImGui::GetIO();
      if (code < countof(io.KeysDown))
        io.KeysDown[code] = true;

      if (!io.WantCaptureKeyboard)
        HandleHostKeyEvent(code, 0, true);
    });
  }

  NativeDPadEvent(args, true);
  // args.Handled(true);
}

void UWPHostInterface::OnKeyUp(const IInspectable&, const winrt::Windows::UI::Core::KeyEventArgs& args)
{
  if (TriggerHook(0, (int)args.VirtualKey(), false))
  {
    return;
  }
  const auto status = args.KeyStatus();
  if (status.WasKeyDown && status.IsKeyReleased && IsEmulationThreadRunning())
  {
    const HostKeyCode code = static_cast<HostKeyCode>(args.VirtualKey());
    RunLater([this, code]() {
      ImGuiIO& io = ImGui::GetIO();
      if (code < countof(io.KeysDown))
        io.KeysDown[code] = false;

      if (!io.WantCaptureKeyboard)
        HandleHostKeyEvent(code, 0, false);
    });
  }

  NativeDPadEvent(args, false);
  // args.Handled(true);
}

void UWPHostInterface::OnCharacterReceived(const IInspectable&,
                                           const winrt::Windows::UI::Core::CharacterReceivedEventArgs& args)
{
  if (IsEmulationThreadRunning())
  {
    const u32 code = args.KeyCode();
    RunLater([this, code]() { ImGui::GetIO().AddInputCharacter(code); });
  }

  args.Handled(true);
}

float tempX = 0.0f;
float tempY = 0.0f;
float temp2X = 0.0f;
float temp2Y = 0.0f;
bool any_scrollbar_active = false;
void UWPHostInterface::IsPointerOverScrollbar()
{
  /*if (ImGui::IsAnyItemActive())
  {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImGuiID active_id = ImGui::GetActiveID();
    if (active_id)
    {
      auto XID = ImGui::GetWindowScrollbarID(window, ImGuiAxis_X);
      auto YID = ImGui::GetWindowScrollbarID(window, ImGuiAxis_Y);
      Log_InfoPrintf("active_id: %d", active_id);
      Log_InfoPrintf("XID: %d", XID);
      Log_InfoPrintf("YID: %d", YID);
      any_scrollbar_active = active_id == XID || active_id == YID;
    }
    else
    {
      Log_InfoPrint("No active id!");
      any_scrollbar_active = false;
    }
  }*/
}

bool IsDraggingItem()
{
  if (ImGui::IsAnyItemActive())
  {
    auto mDelta = ImGui::GetMouseDragDelta();
    if (g_settings.touch_log)
    {
      Log_WarningPrintf("mDeltaX: %.2f, mDeltaY: %.2f", mDelta.x, mDelta.y);
    }
    return true;
  }

  return false;
}

void UWPHostInterface::OnPointerPressed(const IInspectable&, const winrt::Windows::UI::Core::PointerEventArgs& args)
{
  const auto pointer = args.CurrentPoint();
  const auto properties = pointer.Properties();
  const auto pos = pointer.Position();
  auto currentX = pos.X * m_display->GetWindowScale();
  auto currentY = pos.Y * m_display->GetWindowScale();
  if (g_settings.touch_log)
  {
    Log_WarningPrint("--------PRESSED--------");
    Log_WarningPrintf("currentX: %.2f", currentX);
    Log_WarningPrintf("currentY: %.2f", currentY);
  }

  if (ImGui::GetCurrentContext())
  {
    if (g_settings.touch_log)
    {
      Log_WarningPrint("hold!");
    }
    ImGuiIO& io = ImGui::GetIO();
    m_display->SetMousePosition(static_cast<s32>(currentX), static_cast<s32>(currentY));
    io.MousePos.x = currentX;
    io.MousePos.y = currentY;
    io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
  }

  temp2X = currentX;
  temp2Y = currentY;
  tempX = currentX;
  tempY = currentY;

  int id = pointer.PointerId();
  input_pointer newPointer{id, currentX, currentY, pointer.IsInContact()};
  addOrUpdatePointer(newPointer);
  // args.Handled(true);
}

void UWPHostInterface::OnPointerReleased(const IInspectable&, const winrt::Windows::UI::Core::PointerEventArgs& args)
{
  const auto pointer = args.CurrentPoint();
  TouchActive = true;
  const auto properties = pointer.Properties();
  const auto pos = pointer.Position();
  auto currentX = pos.X * m_display->GetWindowScale();
  auto currentY = pos.Y * m_display->GetWindowScale();
  float deltaX = currentX - temp2X;
  float deltaY = currentY - temp2Y;
  float distance = sqrt(deltaX * deltaX + deltaY * deltaY);

  // Update the last pointer position
  temp2X = currentX;
  temp2Y = currentY;
  if (g_settings.touch_log)
  {
    Log_WarningPrint("--------RELEASED--------");
    Log_WarningPrintf("currentX: %.2f", currentX);
    Log_WarningPrintf("currentY: %.2f", currentY);
    Log_WarningPrintf("distance: %.2f", distance);
  }
  // Only update ImGui input if the movement is above the threshold
  if (distance <= touchThreshold)
  {
    if (ImGui::GetCurrentContext())
    {
      if (g_settings.touch_log)
      {
        Log_WarningPrint("clicked!");
      }
      ImGuiIO& io = ImGui::GetIO();
      m_display->SetMousePosition(static_cast<s32>(currentX), static_cast<s32>(currentY));
      io.MousePos.x = currentX;
      io.MousePos.y = currentY;
      io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
    }
  }
  else
  {
    if (ImGui::GetCurrentContext())
    {
      if (g_settings.touch_log)
      {
        Log_WarningPrint("released!");
      }
      ImGuiIO& io = ImGui::GetIO();
      ImGui::BeginDisabled();
      io.MousePos.x = -10;
      io.MousePos.y = -10;
      io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
      ImGui::EndDisabled();
    }
  }

  int id = pointer.PointerId();
  removePointer(id);
  // args.Handled(true);
}

void UWPHostInterface::OnPointerMoved(const IInspectable&, const winrt::Windows::UI::Core::PointerEventArgs& args)
{
  const auto pointer = args.CurrentPoint();

  const auto properties = pointer.Properties();
  const auto pos = pointer.Position();
  auto currentX = pos.X * m_display->GetWindowScale();
  auto currentY = pos.Y * m_display->GetWindowScale();

  bool isPressed = properties.IsLeftButtonPressed();

  if (isPressed)
  {
    // Calculate the distance between the current and last pointer positions
    float deltaX = currentX - tempX;
    float deltaY = currentY - tempY;
    float distance = sqrt(deltaX * deltaX + deltaY * deltaY);

    // Only update ImGui input if the movement is above the threshold
    if (g_settings.touch_log)
    {
      Log_WarningPrint("--------MOVED--------");
      Log_WarningPrintf("currentX: %.2f", currentX);
      Log_WarningPrintf("currentY: %.2f", currentY);
      Log_WarningPrintf("distance: %.2f", distance);
    }
    if (distance > touchThreshold)
    {
      if (ImGui::GetCurrentContext())
      {
        // Calculate the change in pointer position
        float deltaScrollY = (currentY - tempY) * 0.01f;
        if (g_settings.touch_log)
        {
          Log_WarningPrintf("deltaScrollY: %.2f", deltaScrollY);
        }

        // Update ImGui input for scrolling
        ImGuiIO& io = ImGui::GetIO();
        // const float dw = static_cast<float>(std::clamp<s32>(deltaY, -1, 1));
        io.MouseWheel += deltaScrollY; // Adjust the sensitivity as needed

        m_display->SetMousePosition(static_cast<s32>(currentX), static_cast<s32>(currentY));
        io.MousePos.x = currentX;
        io.MousePos.y = currentY;
      }
    }
    else /* if (IsDraggingItem())*/
    {
      if (g_settings.touch_log)
      {
        Log_WarningPrint("moved!");
      }
      ImGuiIO& io = ImGui::GetIO();
      m_display->SetMousePosition(static_cast<s32>(currentX), static_cast<s32>(currentY));
      io.MousePos.x = currentX;
      io.MousePos.y = currentY;
    }

    // Update the last pointer position
    tempX = currentX;
    tempY = currentY;
  }
  if (pointer.IsInContact())
  {
    int id = pointer.PointerId();
    input_pointer newPointer{id, currentX, currentY, pointer.IsInContact()};
    addOrUpdatePointer(newPointer);
  }
  // args.Handled(true);
}

void UWPHostInterface::OnPointerWheelChanged(const IInspectable&,
                                             const winrt::Windows::UI::Core::PointerEventArgs& args)
{
  const auto pointer = args.CurrentPoint();
  const auto properties = pointer.Properties();
  const s32 delta = properties.MouseWheelDelta();
  const bool horizontal = properties.IsHorizontalMouseWheel();

  if (IsEmulationThreadRunning())
  {
    RunLater([this, delta, horizontal]() {
      if (ImGui::GetCurrentContext())
      {
        ImGuiIO& io = ImGui::GetIO();
        const float dw = static_cast<float>(std::clamp<s32>(delta, -1, 1));
        if (horizontal)
          io.MouseWheelH = dw;
        else
          io.MouseWheel = dw;
      }
    });
  }

  args.Handled(true);
}

void UWPHostInterface::OnBackRequested(const IInspectable&,
                                       const winrt::Windows::UI::Core::BackRequestedEventArgs& args)
{
  if (winrt::Windows::Foundation::Metadata::ApiInformation::IsTypePresent(L"Windows.Phone.UI.Input.HardwareButtons"))
  {
    if (TouchActive)
    {
      if (!FullscreenUI::OpenQuickMenu())
      {
        FullscreenUI::ReturnToMainWindow();
      }
    }
  }
  args.Handled(true);
}

std::optional<CommonHostInterface::HostKeyCode> UWPHostInterface::GetHostKeyCode(const std::string_view key_code) const
{
  for (const auto& it : s_key_map)
  {
    if (key_code.compare(it.second) == 0)
      return static_cast<HostKeyCode>(it.first);
  }

  return std::nullopt;
}

const char* UWPHostInterface::GetKeyCodeName(int key_code)
{
  const auto it = s_key_map.find(key_code);
  return (it != s_key_map.end()) ? it->second : nullptr;
}

const char* GetKeyCodeName(int key_code)
{
  const auto it = s_key_map.find(key_code);
  return (it != s_key_map.end()) ? it->second : nullptr;
}

void UWPHostInterface::SetImGuiKeyMap()
{
  using namespace winrt::Windows::System;

  ImGuiIO& io = ImGui::GetIO();
  io.KeyMap[ImGuiKey_Tab] = static_cast<int>(VirtualKey::Tab);
  io.KeyMap[ImGuiKey_LeftArrow] = static_cast<int>(VirtualKey::Left);
  io.KeyMap[ImGuiKey_RightArrow] = static_cast<int>(VirtualKey::Right);
  io.KeyMap[ImGuiKey_UpArrow] = static_cast<int>(VirtualKey::Up);
  io.KeyMap[ImGuiKey_DownArrow] = static_cast<int>(VirtualKey::Down);
  io.KeyMap[ImGuiKey_PageUp] = static_cast<int>(VirtualKey::PageUp);
  io.KeyMap[ImGuiKey_PageDown] = static_cast<int>(VirtualKey::PageDown);
  io.KeyMap[ImGuiKey_Home] = static_cast<int>(VirtualKey::Home);
  io.KeyMap[ImGuiKey_End] = static_cast<int>(VirtualKey::End);
  io.KeyMap[ImGuiKey_Insert] = static_cast<int>(VirtualKey::Insert);
  io.KeyMap[ImGuiKey_Delete] = static_cast<int>(VirtualKey::Delete);
  io.KeyMap[ImGuiKey_Backspace] = static_cast<int>(VirtualKey::Back);
  io.KeyMap[ImGuiKey_Space] = static_cast<int>(VirtualKey::Space);
  io.KeyMap[ImGuiKey_Enter] = static_cast<int>(VirtualKey::Enter);
  io.KeyMap[ImGuiKey_Escape] = static_cast<int>(VirtualKey::Escape);
  io.KeyMap[ImGuiKey_A] = static_cast<int>(VirtualKey::A);
  io.KeyMap[ImGuiKey_C] = static_cast<int>(VirtualKey::C);
  io.KeyMap[ImGuiKey_V] = static_cast<int>(VirtualKey::V);
  io.KeyMap[ImGuiKey_X] = static_cast<int>(VirtualKey::X);
  io.KeyMap[ImGuiKey_Y] = static_cast<int>(VirtualKey::Y);
  io.KeyMap[ImGuiKey_Z] = static_cast<int>(VirtualKey::Z);
}

void UWPHostInterface::SetDefaultSettingsForXbox(SettingsInterface& si)
{
  si.SetStringValue("GPU", "Renderer", "D3D12");

  si.SetBoolValue("Main", "SyncToHostRefreshRate", true);
  si.SetBoolValue("Display", "VSync", true);
  si.SetBoolValue("Display", "DisplayAllFrames", true);
  si.SetFloatValue("Display", "MaxFPS", 60.0f);

  // Set up an analog controller in port 1.
  si.SetStringValue("Controller1", "Type", "AnalogController");
  si.SetStringValue("Controller1", "ButtonUp", "Controller0/Button11");
  si.SetStringValue("Controller1", "ButtonDown", "Controller0/Button12");
  si.SetStringValue("Controller1", "ButtonLeft", "Controller0/Button13");
  si.SetStringValue("Controller1", "ButtonRight", "Controller0/Button14");
  si.SetStringValue("Controller1", "ButtonStart", "Controller0/Button6");
  si.SetStringValue("Controller1", "ButtonTriangle", "Controller0/Button3");
  si.SetStringValue("Controller1", "ButtonCross", "Controller0/Button0");
  si.SetStringValue("Controller1", "ButtonCircle", "Controller0/Button1");
  si.SetStringValue("Controller1", "ButtonSquare", "Controller0/Button2");
  si.SetStringValue("Controller1", "ButtonL1", "Controller0/Button9");
  si.SetStringValue("Controller1", "ButtonL2", "Controller0/+Axis4");
  si.SetStringValue("Controller1", "ButtonR1", "Controller0/Button10");
  si.SetStringValue("Controller1", "ButtonR2", "Controller0/+Axis5");
  si.SetStringValue("Controller1", "ButtonL3", "Controller0/Button7");
  si.SetStringValue("Controller1", "ButtonR3", "Controller0/Button8");
  si.SetStringValue("Controller1", "AxisLeftX", "Controller0/Axis0");
  si.SetStringValue("Controller1", "AxisLeftY", "Controller0/Axis1");
  si.SetStringValue("Controller1", "AxisRightX", "Controller0/Axis2");
  si.SetStringValue("Controller1", "AxisRightY", "Controller0/Axis3");
  si.SetStringValue("Controller1", "Rumble", "Controller0");
  si.SetStringValue("Controller1", "ForceAnalogOnReset", "true");
  si.SetStringValue("Controller1", "AnalogDPadInDigitalMode", "true");

  // Repurpose the select button to open the menu.
  // Not ideal, but all we can do until we have chords.
  si.SetStringValue("Hotkeys", "OpenQuickMenu", "Controller0/Button4");
}

void UWPHostInterface::SetDefaultSettingsForARM(SettingsInterface& si)
{
  // si.SetBoolValue("Main", "SyncToHostRefreshRate", true);
  // si.SetBoolValue("Display", "VSync", true);
  // si.SetBoolValue("Display", "DisplayAllFrames", true);
  // si.SetFloatValue("Display", "MaxFPS", 60.0f);
  si.SetBoolValue("Main", "StartFullscreen", true);
  si.SetBoolValue("Main", "InhibitScreensaver", false);
  si.SetBoolValue("Cheevos", "RichPresence", false);

  si.SetStringValue("Controller1", "ButtonUp", "Keyboard/Up");
  si.SetStringValue("Controller1", "ButtonDown", "Keyboard/Down");
  si.SetStringValue("Controller1", "ButtonLeft", "Keyboard/Left");
  si.SetStringValue("Controller1", "ButtonRight", "Keyboard/Right");
  si.SetStringValue("Controller1", "ButtonSelect", "Keyboard/Backspace");
  si.SetStringValue("Controller1", "ButtonStart", "Keyboard/Return");
  si.SetStringValue("Controller1", "ButtonTriangle", "Keyboard/A");
  si.SetStringValue("Controller1", "ButtonCross", "Keyboard/X");
  si.SetStringValue("Controller1", "ButtonCircle", "Keyboard/S");
  si.SetStringValue("Controller1", "ButtonSquare", "Keyboard/Z");
  si.SetStringValue("Controller1", "ButtonL1", "Keyboard/Q");
  si.SetStringValue("Controller1", "ButtonL2", "Keyboard/1");
  si.SetStringValue("Controller1", "ButtonR1", "Keyboard/E");
  si.SetStringValue("Controller1", "ButtonR2", "Keyboard/3");
  si.SetStringValue("Hotkeys", "OpenQuickMenu", "Controller0/Button4");
}

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
  winrt::Windows::ApplicationModel::Core::CoreApplication::Run(winrt::make<UWPHostInterface>());
}
