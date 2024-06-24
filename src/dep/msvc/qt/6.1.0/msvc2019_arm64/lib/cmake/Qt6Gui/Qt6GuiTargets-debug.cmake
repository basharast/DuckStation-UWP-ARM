#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Qt6::Gui" for configuration "Debug"
set_property(TARGET Qt6::Gui APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(Qt6::Gui PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/lib/Qt6Guid.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/Qt6Guid.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS Qt6::Gui )
list(APPEND _IMPORT_CHECK_FILES_FOR_Qt6::Gui "${_IMPORT_PREFIX}/lib/Qt6Guid.lib" "${_IMPORT_PREFIX}/bin/Qt6Guid.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
