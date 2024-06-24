#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Qt6::Svg" for configuration "Release"
set_property(TARGET Qt6::Svg APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Qt6::Svg PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/Qt6Svg.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/Qt6Svg.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS Qt6::Svg )
list(APPEND _IMPORT_CHECK_FILES_FOR_Qt6::Svg "${_IMPORT_PREFIX}/lib/Qt6Svg.lib" "${_IMPORT_PREFIX}/bin/Qt6Svg.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
