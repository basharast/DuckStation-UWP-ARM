#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Qt6::QSvgPlugin" for configuration "Debug"
set_property(TARGET Qt6::QSvgPlugin APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(Qt6::QSvgPlugin PROPERTIES
  IMPORTED_COMMON_LANGUAGE_RUNTIME_DEBUG ""
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/./plugins/imageformats/qsvgd.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS Qt6::QSvgPlugin )
list(APPEND _IMPORT_CHECK_FILES_FOR_Qt6::QSvgPlugin "${_IMPORT_PREFIX}/./plugins/imageformats/qsvgd.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
