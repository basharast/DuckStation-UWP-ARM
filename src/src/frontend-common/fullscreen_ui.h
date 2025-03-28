#pragma once
#include "common/types.h"
#include <string>
#include <memory>

class HostDisplayTexture;
class CommonHostInterface;
class SettingsInterface;
struct Settings;

namespace FrontendCommon {
enum class ControllerNavigationButton : u32;
}

namespace FullscreenUI {
enum class MainWindowType
{
  None,
  Landing,
  GameList,
  Settings,
  QuickMenu,
  Achievements,
  Leaderboards,
};

enum class SettingsPage
{
  InterfaceSettings,
  GameListSettings,
  ConsoleSettings,
  EmulationSettings,
  BIOSSettings,
  ControllerSettings,
  HotkeySettings,
  MemoryCardSettings,
  DisplaySettings,
  EnhancementSettings,
  AudioSettings,
  AchievementsSetings,
  AdvancedSettings,
  Count
};

bool Initialize(CommonHostInterface* host_interface);
bool IsInitialized();
bool HasActiveWindow();
void UpdateSettings();
void SystemCreated();
void SystemDestroyed();
bool OpenQuickMenu();
void CloseQuickMenu();
void ClearImGuiFocus();
void DoStartFile2(std::string dpath);
void DoStartFile();
void ChangeDataFolder();
void DoChangeDiscFromFile();
void DoAddSearchDir();
void DoSettingInfoButton();
void DoBIOSDirectory();
void DoMemCardDirectory();
void DoPowerOff();
void DoNothing();
void DrawAboutWindow();
void DrawCompatibilityWindow();
void DrawRestartWindow();
void DoStartPath(const std::string& path, bool allow_resume);


#ifdef WITH_CHEEVOS
bool OpenAchievementsWindow();
bool OpenLeaderboardsWindow();
#endif

void Shutdown();
void Render();

bool IsBindingInput();
bool HandleKeyboardBinding(const char* keyName, bool pressed);
bool HandleKeyboardBinding(int code, bool pressed);

std::unique_ptr<HostDisplayTexture> LoadTextureResource(const char* name, bool allow_fallback = true);

// Returns true if the message has been dismissed.
bool DrawErrorWindow(const char* message);
bool DrawConfirmWindow(const char* message, bool* result);

void QueueGameListRefresh();
void EnsureGameListLoaded();

Settings& GetSettingsCopy();
void SaveAndApplySettings();
void SetDebugMenuAllowed(bool allowed);
void ReturnToMainWindow();

/// Only ImGuiNavInput_Activate, ImGuiNavInput_Cancel, and DPad should be forwarded.
/// Returns true if the UI consumed the event, and it should not execute the normal handler.
bool SetControllerNavInput(FrontendCommon::ControllerNavigationButton button, bool value);

/// Forwards the controller navigation to ImGui for fullscreen navigation. Call before NewFrame().
void SetImGuiNavInputs();

} // namespace FullscreenUI
