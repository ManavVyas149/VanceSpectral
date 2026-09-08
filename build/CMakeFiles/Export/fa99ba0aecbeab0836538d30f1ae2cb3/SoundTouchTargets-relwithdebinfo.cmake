#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "SoundTouch::SoundTouch" for configuration "RelWithDebInfo"
set_property(TARGET SoundTouch::SoundTouch APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(SoundTouch::SoundTouch PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "CXX"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/SoundTouch.lib"
  )

list(APPEND _cmake_import_check_targets SoundTouch::SoundTouch )
list(APPEND _cmake_import_check_files_for_SoundTouch::SoundTouch "${_IMPORT_PREFIX}/lib/SoundTouch.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
