// UWP STORAGE MANAGER
// Copyright (c) 2023 Bashar Astifan.
// Email: bashar@astifan.online
// Telegram: @basharastifan
// GitHub: https://github.com/basharast/UWP2Win32

// This code must keep support for lower builds (15063+)
// Try always to find possible way to keep that support

#include "StorageConfig.h"
#include "StorageLog.h"
#include "StorageExtensions.h"
#include "StorageAsync.h"
#include "StorageAccess.h"
#include "StorageItemW.h"

#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Storage.AccessCache.h>

using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Storage::AccessCache;
using namespace winrt::Windows::ApplicationModel;
using namespace winrt::Windows::UI::Core;

// Main lookup list
std::list<StorageItemW> FutureAccessItems;


// Add item to history list (FutureAccessItems)
void AddToAccessibleItems(IStorageItem item) {
	bool isFolderAddedBefore = false;
	for (auto folderItem : FutureAccessItems) {
		if (folderItem.Equal(item)) {
			isFolderAddedBefore = true;
			break;
		}
	}

	if (!isFolderAddedBefore) {
		FutureAccessItems.push_back(StorageItemW(item));
	}
}


// Add folder to future list (to avoid request picker again)
void AddItemToFutureList(IStorageItem item) {
	try {
		if (item != nullptr) {
			winrt::hstring folderToken = AccessCache::StorageApplicationPermissions::FutureAccessList().Add(item);
			AddToAccessibleItems(item);
		}
	}
	catch (...) {
	}
}

// Append folder by token to (FutureAccessFolders)
void AppendItemByToken(winrt::hstring token) {
	try {
		if (!token.empty() && AccessCache::StorageApplicationPermissions::FutureAccessList().ContainsItem(token)) {
			IStorageItem storageItem;
			ExecuteTask(storageItem, AccessCache::StorageApplicationPermissions::FutureAccessList().GetItemAsync(token));
			AddToAccessibleItems(storageItem);
		}
	}
	catch (...) {
	}
}

// Update the history list by the future list (to restore all the picked items)
void UpdateItemsByFutureList() {
	auto AccessList = AccessCache::StorageApplicationPermissions::FutureAccessList().Entries();
	for (auto ListItem : AccessList) {
		winrt::hstring itemToken = ListItem.Token;
		AppendItemByToken(itemToken);
	}
}

bool fillListsCalled = false;
bool fillListInProgress = false;
void FillLookupList() {
	if (fillListsCalled) {
		// Should be called only once
		// but let's wait in case we got too calls at once
		CoreWindow corewindow = CoreWindow::GetForCurrentThread();
		while (fillListInProgress)
		{
			try {
				if (corewindow) {
					corewindow.Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
				}
				else {
					corewindow = CoreWindow::GetForCurrentThread();
				}
			}
			catch (...) {

			}
		}
		return;
	}
	fillListsCalled = true;
	fillListInProgress = true;
	// Clean access list from any deleted/moved items
  auto filesList = AccessCache::StorageApplicationPermissions::FutureAccessList().Entries();
  for (auto it = 0; it != filesList.Size(); ++it)
  {
    AccessCache::AccessListEntry listItem = filesList.GetAt(it);
		try {
      IStorageItem test =
        AccessCache::StorageApplicationPermissions::FutureAccessList().GetItemAsync(listItem.Token).GetResults();
			//ExecuteTask(test, AccessCache::StorageApplicationPermissions::FutureAccessList().GetItemAsync(listItem.Token));
			if (test == nullptr) {
				// Access denied or file moved/deleted
				AccessCache::StorageApplicationPermissions::FutureAccessList().Remove(listItem.Token);
			}
		}
		catch (...) {
			// Access denied or file moved/deleted
			AccessCache::StorageApplicationPermissions::FutureAccessList().Remove(listItem.Token);
		}
	}

	// Get files/folders selected by the user
	UpdateItemsByFutureList();

	// Append known folders
#if APPEND_APP_LOCALDATA_LOCATION
	AddToAccessibleItems(ApplicationData::Current().LocalFolder());
	AddToAccessibleItems(ApplicationData::Current().TemporaryFolder());
#endif

#if APPEND_APP_INSTALLATION_LOCATION
	AddToAccessibleItems(Package::Current().InstalledLocation());
#endif

#if APPEND_DOCUMENTS_LOCATION
	// >>>>DOCUMENTS (requires 'documentsLibrary' capability)
	AddToAccessibleItems(KnownFolders::DocumentsLibrary());
#endif

#if APPEND_VIDEOS_LOCATION
	// >>>>VIDEOS (requires 'videosLibrary' capability)
	AddToAccessibleItems(KnownFolders::VideosLibrary());
#endif

#if APPEND_PICTURES_LOCATION
	// >>>>VIDEOS (requires 'picturesLibrary' capability)
	AddToAccessibleItems(KnownFolders::PicturesLibrary());
#endif

#if APPEND_MUSIC_LOCATION
	// >>>>MUSIC (requires 'musicLibrary' capability)
	AddToAccessibleItems(KnownFolders::MusicLibrary());
#endif

	// No need to append `RemovableDevices`
	// they will be allowed for access once you added the capability

	fillListInProgress = false;
}
