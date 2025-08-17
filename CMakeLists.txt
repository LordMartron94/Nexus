cmake_minimum_required(VERSION 3.28)

# --- Project meta (filled by your generator) ---
project(nexus
  VERSION 0.0.0
  DESCRIPTION "C library: nexus"
  LANGUAGES C
)

# C standard
set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Options
option(nexus_BUILD_APP   "Build demo console app (src/main.c)" ON)
option(nexus_BUILD_TESTS "Build tests in /tests"               ON)
option(BUILD_SHARED_LIBS           "Build shared instead of static"      OFF)

# Use canonical install dirs (handles lib vs lib64, etc.)
include(GNUInstallDirs)

# ---- Library ----
add_library(nexus
  src/nexus.c
)
add_library(nexus::nexus ALIAS nexus)

# Position-independent code (useful if the static lib is later linked into a shared lib)
set_target_properties(nexus PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  VERSION   0.0.0
  SOVERSION 0
)

# Public headers for build-tree and install-tree consumers
target_include_directories(nexus
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# (Optional) warnings on MSVC/clang-cl
if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
  target_compile_options(nexus PRIVATE /W4)
elseif (MSVC)
  target_compile_options(nexus PRIVATE /W4)
endif()

# ---- Demo app (optional) ----
if(nexus_BUILD_APP)
  add_executable(nexus_app src/main.c)
  target_link_libraries(nexus_app PRIVATE nexus::nexus)
endif()

# ---- Tests (optional & minimal) ----
if(nexus_BUILD_TESTS)
  enable_testing()
  add_executable(nexus_tests tests/entry.c)
  target_link_libraries(nexus_tests PRIVATE nexus::nexus)
  add_test(NAME nexus.basic COMMAND nexus_tests)
endif()

# ---- Install & export (find_package support) ----
include(CMakePackageConfigHelpers)

install(TARGETS nexus
  EXPORT nexusTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/nexus-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/nexus-config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/nexus
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/nexus-config-version.cmake
  VERSION 0.0.0
  COMPATIBILITY SameMajorVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/nexus-config.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/nexus-config-version.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/nexus
)

install(EXPORT nexusTargets
  NAMESPACE nexus::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/nexus
)

# Support build-tree find_package when using add_subdirectory in a superproject
export(EXPORT nexusTargets
  NAMESPACE nexus::
  FILE ${CMAKE_CURRENT_BINARY_DIR}/nexusTargets.cmake
)
