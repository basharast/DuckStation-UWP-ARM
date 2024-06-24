#pragma once
#include "common/event.h"
#include "common/window_info.h"
#include "common/windows_headers.h"
#include "core/host_display.h"
#include "core/host_interface.h"
#include "frontend-common/common_host_interface.h"
#include <Unknwn.h>
#include <array>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include "imgui.h"

#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.Devices.Input.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Display.h>
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.UI.Input.h>
#include <winrt/Windows.UI.ViewManagement.Core.h>
#include <winrt/Windows.UI.ViewManagement.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.ApplicationModel.Activation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Gaming.Input.h>
#include <winrt/Windows.Devices.Sensors.h>
#include <winrt/base.h>

class INISettingsInterface;

class UWPHostInterface
  : public CommonHostInterface,
    public winrt::implements<UWPHostInterface, winrt::Windows::ApplicationModel::Core::IFrameworkViewSource,
                             winrt::Windows::ApplicationModel::Core::IFrameworkView>
{
public:
  enum class ButtonsX : u8
  {
    Select = 0,
    L3 = 1,
    R3 = 2,
    Start = 3,
    Up = 4,
    Right = 5,
    Down = 6,
    Left = 7,
    L2 = 8,
    R2 = 9,
    L1 = 10,
    R1 = 11,
    Triangle = 12,
    Circle = 13,
    Cross = 14,
    Square = 15,
    Count
  };
  enum class ButtonsA : u8
  {
    Select = 0,
    L3 = 1,
    R3 = 2,
    Start = 3,
    Up = 4,
    Right = 5,
    Down = 6,
    Left = 7,
    L2 = 8,
    R2 = 9,
    L1 = 10,
    R1 = 11,
    Triangle = 12,
    Circle = 13,
    Cross = 14,
    Square = 15,
    Analog = 16,
    Count
  };

  struct TouchButton
  {
    const char* title;
    ImVec2* position;
    ImVec2* size;
    bool state;
    ButtonsX trigger;
    ButtonsA triggera;
    float x_extended = 0.0f;
    float y_extended = 0.0f;
    float px_extended = 0.0f;
    float py_extended = 0.0f;
    float x_extended_a = 0.0f;
    float y_extended_a = 0.0f;
    float px_extended_a = 0.0f;
    float py_extended_a = 0.0f;
  };

  enum class Axis : u8
  {
    LeftX,
    LeftY,
    RightX,
    RightY,
    Count
  };

  UWPHostInterface();
  ~UWPHostInterface();

  const char* GetFrontendName() const override;
  bool Initialize() override;
  void Shutdown() override;

  void ReportMessage(const char* message) override;
  void ReportError(const char* message) override;
  bool ConfirmMessage(const char* message) override;
  void RenderTouchControls();
  void OnOrientationChanged(winrt::Windows::Graphics::Display::DisplayInformation sender,
                           const winrt::Windows::Foundation::IInspectable& args);
 
  void TriggerButton(int code, bool pressed);
  void TriggerButton(int port, int code, bool pressed, bool analog = false, Axis type = Axis::Count, float value = 0.0f);
  bool TriggerHook(int port, int code, bool pressed, bool analog = false, Axis type = Axis::Count, float value = 0.0f);
  void UpdateTouchButtonsState();

  void RunLater(std::function<void()> callback) override;

  void OnDisplayInvalidated() override;
  void OnSystemPerformanceCountersUpdated() override;

  bool IsFullscreen() const override;
  bool SetFullscreen(bool enabled) override;

  winrt::Windows::ApplicationModel::Core::IFrameworkView CreateView();
  void Initialize(const winrt::Windows::ApplicationModel::Core::CoreApplicationView&);
  void Load(const winrt::hstring&);
  void Uninitialize();
  void Run();
  void SetWindow(const winrt::Windows::UI::Core::CoreWindow& window);
  void updateButtonsMap2();

protected:
  enum : u32
  {
    DEFAULT_WINDOW_WIDTH = 1280,
    DEFAULT_WINDOW_HEIGHT = 720
  };

  ALWAYS_INLINE bool IsEmulationThreadRunning() const { return m_emulation_thread.joinable(); }

  void SetDefaultSettings(SettingsInterface& si) override;

  bool AcquireHostDisplay() override;
  void ReleaseHostDisplay() override;

  void PollAndUpdate() override;
  void RequestExit() override;

  void SetMouseMode(bool relative, bool hide_cursor) override;

  bool CreateDisplay(bool fullscreen);
  void DestroyDisplay();
  void RunCallbacks();
  void EmulationThreadEntryPoint();

  void CallRight(bool state = true);
  void CallLeft(bool state = true);
  void CallUp(bool state = true);
  void CallDown(bool state = true);
  bool SetDirectories();
  void IsPointerOverScrollbar();
  void OnSuspending(const IInspectable&, const winrt::Windows::ApplicationModel::SuspendingEventArgs& args);
  void OnResuming(const IInspectable&, const IInspectable&);
  void OnClosed(const IInspectable&, const winrt::Windows::UI::Core::CoreWindowEventArgs& args);
  void OnSizeChanged(const IInspectable&, const winrt::Windows::UI::Core::WindowSizeChangedEventArgs& args);
  void OnKeyDown(const IInspectable&, const winrt::Windows::UI::Core::KeyEventArgs& args);
  void OnKeyUp(const IInspectable&, const winrt::Windows::UI::Core::KeyEventArgs& args);
  void OnCharacterReceived(const IInspectable&, const winrt::Windows::UI::Core::CharacterReceivedEventArgs& args);
  void OnPointerPressed(const IInspectable&, const winrt::Windows::UI::Core::PointerEventArgs& args);
  void OnPointerReleased(const IInspectable&, const winrt::Windows::UI::Core::PointerEventArgs& args);
  void OnPointerMoved(const IInspectable&, const winrt::Windows::UI::Core::PointerEventArgs& args);
  void OnPointerWheelChanged(const IInspectable&, const winrt::Windows::UI::Core::PointerEventArgs& args);
  void OnBackRequested(const IInspectable&, const winrt::Windows::UI::Core::BackRequestedEventArgs& args);
  void OnActivated(const IInspectable&, const winrt::Windows::ApplicationModel::Activation::IActivatedEventArgs& args);
  void OnVisibilityChanged(const IInspectable&,
                           const winrt::Windows::UI::Core::VisibilityChangedEventArgs& args);
  void OnReadingChanged(winrt::Windows::Devices::Sensors::Accelerometer const&,
                        winrt::Windows::Devices::Sensors::AccelerometerReadingChangedEventArgs const& args);
  void LinkAccelerometer();
  void NativeDPadEvent(const winrt::Windows::UI::Core::KeyEventArgs& args, bool buttonState);
  void PollGamingInput();
  void PollGamingRawInput();
  void InitializeGamepad();
  void UnregisterGamepadEvents();
  void RegisterGamepadEvents();
  void GamePadAddedEventHandler(winrt::Windows::Foundation::IInspectable const& sender,
                                winrt::Windows::Gaming::Input::Gamepad const& gamepad);
  void GamePadRemovedEventHandler(winrt::Windows::Foundation::IInspectable const& sender,
                                  winrt::Windows::Gaming::Input::Gamepad const& gamepad);
  void updateButtonState(winrt::Windows::System::VirtualKey targetKey, bool state);


  std::optional<HostKeyCode> GetHostKeyCode(const std::string_view key_code) const override;
  const char* GetKeyCodeName(int key_code);
  void SetImGuiKeyMap();

  void SetDefaultSettingsForXbox(SettingsInterface& si);
  void SetDefaultSettingsForARM(SettingsInterface& si);

  std::deque<std::function<void()>> m_queued_callbacks;
  std::mutex m_queued_callbacks_lock;

  winrt::Windows::UI::Core::CoreWindow m_window{nullptr};
  winrt::Windows::UI::Core::CoreDispatcher m_dispatcher{nullptr};
  winrt::Windows::UI::ViewManagement::ApplicationView m_appview{nullptr};

  std::thread m_emulation_thread;
  std::atomic_bool m_shutdown_flag{false};

  bool m_text_input_requested = false;

  Common::Event m_suspend_sync_event;
  std::atomic_bool m_was_running_on_suspend{false};
  std::atomic_bool m_was_running_on_background{false};
};
