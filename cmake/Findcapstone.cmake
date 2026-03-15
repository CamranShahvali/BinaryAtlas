find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(PC_CAPSTONE QUIET capstone)
endif()

find_path(
  capstone_INCLUDE_DIR
  NAMES capstone/capstone.h
  HINTS ${PC_CAPSTONE_INCLUDE_DIRS}
  PATH_SUFFIXES include
)

find_library(
  capstone_LIBRARY
  NAMES capstone
  HINTS ${PC_CAPSTONE_LIBRARY_DIRS}
  PATH_SUFFIXES lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  capstone
  REQUIRED_VARS capstone_LIBRARY capstone_INCLUDE_DIR
)

if(capstone_FOUND AND NOT TARGET capstone::capstone)
  add_library(capstone::capstone UNKNOWN IMPORTED)
  set_target_properties(
    capstone::capstone
    PROPERTIES
      IMPORTED_LOCATION "${capstone_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${capstone_INCLUDE_DIR}"
  )
endif()
