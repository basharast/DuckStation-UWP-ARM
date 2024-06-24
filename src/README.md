# DuckStation - PlayStation 1, aka. PSX Emulator

Duckstation fork to keep the original duckstation uwp build available for everyone. 

**UWP Build for Xbox** https://github.com/errory2k/duckstation_uwp/releases/tag/uwp_release

**Latest Builds for Windows and Linux (AppImage)** https://github.com/stenzek/duckstation/releases/tag/latest

**Game Compatibility List:** https://docs.google.com/spreadsheets/d/1H66MxViRjjE5f8hOl5RQmF5woS1murio2dsLn14kEqo/edit

**Wiki:** https://www.duckstation.org/wiki/

DuckStation is an simulator/emulator of the Sony PlayStation(TM) console, focusing on playability, speed, and long-term maintainability. The goal is to be as accurate as possible while maintaining performance suitable for low-end devices. "Hack" options are discouraged, the default configuration should support all playable games with only some of the enhancements having compatibility issues.

A "BIOS" ROM image is required to to start the emulator and to play games. You can use an image from any hardware version or region, although mismatching game regions and BIOS regions may have compatibility issues. A ROM image is not provided with the emulator for legal reasons, you should dump this from your own console using Caetla or other means.

### Universal Windows Platform / Xbox One

The DuckStation fullscreen UI is available for the Universal Windows Platform and Xbox One.

To use on Xbox One:

1. Ensure your console is in developer mode. You will need to purchase a developer license from Microsoft.
2. Download the .appx file.
3. Navigate to the device portal for your console (displayed in the home screen).
4. Install the appx file by clicking Add in the main page.
5. Set the app to Game mode instead of App mode: Scroll down to DuckStation in the listinng, press the `Change View` button, select `View Details`, and change `App` to `Game`.
6. Upload a BIOS image to the local state directory for DuckStation, or place your BIOS image on a removable USB drive. If using a USB drive, you will need to set the BIOS path in DuckStation's settings to point to this directory.
7. Add games to the local state games directory, or use a removable USB drive. Again, you will have to register this path in Game List Settings for it to scan.
8. Launch the app, and enjoy. By default, the `Change View` button will open the quick menu.
9. Don't forget to enable enhancements, an Xbox One S can do 8x resolution scale with 4K output, Series consoles can go higher.

**NOTE:** I'd recommend using a USB drive for saving memory cards, as the local state directory will be removed when you uninstall the app.

## Disclaimers

Icon by icons8: https://icons8.com/icon/74847/platforms.undefined.short-title

"PlayStation" and "PSX" are registered trademarks of Sony Interactive Entertainment Europe Limited. This project is not affiliated in any way with Sony Interactive Entertainment.
