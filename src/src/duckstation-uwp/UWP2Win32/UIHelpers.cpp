// UWP UI HELPER
// Copyright (c) 2023 Bashar Astifan.
// Email: bashar@astifan.online
// Telegram: @basharastifan
// GitHub: https://github.com/basharast/UWP2Win32

#include "UIHelpers.h"

#include <winrt/Windows.System.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.UI.ViewManagement.h>
#include <winrt/Windows.UI.Notifications.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.Data.Xml.Dom.h>

using namespace winrt::Windows::System;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::UI::ViewManagement;
using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::Data::Xml::Dom;
using namespace winrt::Windows::UI::Notifications;

#pragma region Input Keyboard
void ShowInputKeyboard() {
	InputPane::GetForCurrentView().TryShow();
}

void HideInputKeyboard() {
	InputPane::GetForCurrentView().TryHide();
}
#pragma endregion

#pragma region Keys Status
bool IsCapsLockOn() {
	auto capsLockState = CoreApplication::MainView().CoreWindow().GetKeyState(VirtualKey::CapitalLock);
	return (capsLockState == CoreVirtualKeyStates::Locked);
}
bool IsShiftOnHold() {
	auto shiftState = CoreApplication::MainView().CoreWindow().GetKeyState(VirtualKey::Shift);
	return (shiftState == CoreVirtualKeyStates::Down);
}
bool IsCtrlOnHold() {
	auto ctrlState = CoreApplication::MainView().CoreWindow().GetKeyState(VirtualKey::Control);
	return (ctrlState == CoreVirtualKeyStates::Down);
}
#pragma endregion

#pragma region Notifications
void ShowToastNotification(std::string title, std::string message) {
	ToastNotifier toastNotifier = ToastNotificationManager::CreateToastNotifier();
	XmlDocument toastXml = ToastNotificationManager::GetTemplateContent(ToastTemplateType::ToastText02);
	XmlNodeList toastNodeList = toastXml.GetElementsByTagName(L"text");
	toastNodeList.Item(0).AppendChild(toastXml.CreateTextNode(convert(title)));
	toastNodeList.Item(1).AppendChild(toastXml.CreateTextNode(convert(message)));
	IXmlNode toastNode = toastXml.SelectSingleNode(L"/toast");
	XmlElement audio = toastXml.CreateElement(L"audio");
	audio.SetAttribute(L"src", L"ms-winsoundevent:Notification.SMS");
	ToastNotification toast{ ToastNotification(toastXml) };
	toastNotifier.Show(toast);
}
#pragma endregion


