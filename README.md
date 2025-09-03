> ⚠️⚠️⚠️

> This project no longer maintained nor within my interests!.<br/>
> I'm moving toward [ImMobile](https://github.com/basharast/ImMobile) environment and [ImLibretro](https://github.com/basharast/ImLibretro).

---

# DuckStation-UWP (ARM32)
Fast PlayStation 1 emulator for Windows (ARM32) UWP


## Compatibility 
- Windows Build (15035+)
- DirectX 11 Feature Level 10+
- DirectX 11 Feature Level 9.1 (Partially, Software render)
- Don't enable recompiler option still not working

## ARM64 or Latest
- This repo for ARM32, legacy support only
- Please resolve the ARM64 or whatsoever with the original developer not here.
- Please don't ask or contact me for other than ARM32
- This repo is not meant to be up-to date fork (nor redistribution)

## XBOX (x64)
Please visit [XBOX DevStore](https://xbdev.store/)

## Performance
After few tests it worked good on `Adreno 330` but slow on `Adreno 305`

You may achieve better performance by manipulating Speed options

GPUs `Adreno 400+` are perfect and they will work with full speed.

## Screenshots
<details>
<summary>Screenshots</summary>
<img src="https://github.com/basharast/DuckStation-UWP-ARM/assets/3244951/2b1b1f81-2d41-45bf-8862-0c010725cab8" width="760"/>
<img src="https://github.com/basharast/DuckStation-UWP-ARM/assets/3244951/5fd26bb4-3a2c-477c-922a-4a0284dd8a7f" width="250"/>
<img src="https://github.com/basharast/DuckStation-UWP-ARM/assets/3244951/8637920b-34e6-4da3-bf1e-0ac4ff687a62" width="250"/>
<img src="https://github.com/basharast/DuckStation-UWP-ARM/assets/3244951/b1e61420-86f9-4816-ba7a-9ac39a404c52" width="250"/>
</details>


## UWP Support
Supported by **[UWP2Win32](https://github.com/basharast/UWP2Win32)**


## What's new?

- UWP Storage solution
- You can choose custom data folder
- Include and improved touch screen functions
- Added On-Screen touch controller
- Improved files browser
- Fixed few issues with retroachievements
- Other minor fixes related to (Windows Legacy builds, Touch and UWP)
- Support launch by file or URI


## Help

Read about features and performance tips ([Click here](https://github.com/basharast/DuckStation-UWP-ARM/wiki))


# Build 

Ensure to have:
- SDK: 10240, 17763, 19041
- Visual Studio 2022 ( with platform tools v143)
- Only use ReleaseUWP (tested for ARM, ARM64 and x64)

## Important

Some linked projects at Dependencies require to be build manually

like project `util`, I guess also `fmt`, in anycase you got that `.lib` is missing

go to Dependencies and build it manually (Right click, Build)

