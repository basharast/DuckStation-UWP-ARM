host_build {
    QT_ARCH = x86_64
    QT_BUILDABI = x86_64-little_endian-lp64
    QT_TARGET_ARCH = arm64
    QT_TARGET_BUILDABI = arm64-little_endian-lp64
} else {
    QT_ARCH = arm64
    QT_BUILDABI = arm64-little_endian-lp64
    QT_LIBCPP_ABI_TAG = 
}
QT.global.enabled_features = shared cross_compile debug_and_release c++11 c++14 c++17 c++1z c99 c11 signaling_nan thread future concurrent dbus shared cross_compile shared debug_and_release c++11 c++14 c++17 c++1z
QT.global.disabled_features = static pkg-config separate_debug_info appstore-compliant simulator_and_device rpath force_asserts framework c++20 c++2a
QT.global.disabled_features += release build_all
QT_CONFIG += shared no-pkg-config debug_and_release c++11 c++14 c++17 c++1z release debug
CONFIG +=  shared cross_compile plugin_manifest
QT_VERSION = 6.1.0
QT_MAJOR_VERSION = 6
QT_MINOR_VERSION = 1
QT_PATCH_VERSION = 0

QT_MSVC_MAJOR_VERSION = 19
QT_MSVC_MINOR_VERSION = 28
QT_MSVC_PATCH_VERSION = 29915
QT_EDITION = Open Source
