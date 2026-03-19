#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "qpdf::libqpdf" for configuration "RelWithDebInfo"
set_property(TARGET qpdf::libqpdf APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(qpdf::libqpdf PROPERTIES
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/x86_64-linux-gnu/libqpdf.so.30.2.0"
  IMPORTED_SONAME_RELWITHDEBINFO "libqpdf.so.30"
  )

list(APPEND _cmake_import_check_targets qpdf::libqpdf )
list(APPEND _cmake_import_check_files_for_qpdf::libqpdf "${_IMPORT_PREFIX}/lib/x86_64-linux-gnu/libqpdf.so.30.2.0" )

# Import target "qpdf::libqpdf_static" for configuration "RelWithDebInfo"
set_property(TARGET qpdf::libqpdf_static APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(qpdf::libqpdf_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "CXX"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/x86_64-linux-gnu/libqpdf.a"
  )

list(APPEND _cmake_import_check_targets qpdf::libqpdf_static )
list(APPEND _cmake_import_check_files_for_qpdf::libqpdf_static "${_IMPORT_PREFIX}/lib/x86_64-linux-gnu/libqpdf.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
