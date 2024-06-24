// UWP STORAGE MANAGER
// Copyright (c) 2023 Bashar Astifan.
// Email: bashar@astifan.online
// Telegram: @basharastifan

// This code must keep support for lower builds (15063+)
// Try always to find possible way to keep that support

#include "pch.h"

#include "StorageLog.h"
#include "StorageExtensions.h"
#include "StorageAsync.h"
#include "StorageAccess.h"

#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Pickers.h>
#include <winrt/Windows.Foundation.h>

using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Foundation;

extern void AddItemToFutureList(IStorageItem folder);

// Call folder picker (the selected folder will be added to future list)
winrt::hstring PickSingleFolder()
{
	auto folderPicker{ Pickers::FolderPicker() };
	folderPicker.SuggestedStartLocation(Pickers::PickerLocationId::Desktop);
	folderPicker.FileTypeFilter().Append(L"*");

	auto folder = WaitTask(folderPicker.PickSingleFolderAsync());
	winrt::hstring path = {};
	if (folder != nullptr)
	{
		AddItemToFutureList(folder);
		path = folder.Path();
	}
	return path;
}

// Call file picker (the selected file will be added to future list)
winrt::hstring PickSingleFile(std::vector<std::string> exts)
{
	auto filePicker{ Pickers::FileOpenPicker() };
	filePicker.SuggestedStartLocation(Pickers::PickerLocationId::Desktop);
	filePicker.ViewMode(Pickers::PickerViewMode::List);

	if (exts.size() > 0) {
		for (auto ext : exts) {
			filePicker.FileTypeFilter().Append(convert(ext));
		}
	}
	else
	{
    filePicker.FileTypeFilter().Append(convert(std::string("*")));
	}
	auto file = WaitTask(filePicker.PickSingleFileAsync());

	winrt::hstring path = {};
	if (file != nullptr)
	{
		AddItemToFutureList(file);
		path = file.Path();
	}
	return path;
}


std::string ChooseFile(std::vector<std::string> exts) {
	return convert(PickSingleFile(exts));
}

std::string ChooseFolder() {
	return convert(PickSingleFolder());
}
