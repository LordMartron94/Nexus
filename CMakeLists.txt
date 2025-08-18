cmake_minimum_required(VERSION 3.28)
cmake_policy(SET CMP0077 NEW) # honor cache var overrides in subprojects

# --- Project meta ---
project(nexus
        VERSION 0.0.0
        DESCRIPTION "C library: nexus"
        LANGUAGES C
)

# C standard
set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# ===== Nexus feature toggles =====
option(NEXUS_DOUBLE_PRECISION            "Use double for float_real"                    OFF)
option(NEXUS_MEMORY_DEBUG                "Enable debug allocator hooks"                 OFF)  # auto-ON in Debug (see below)
option(NEXUS_OVERRIDE_STDLIB_ALLOC       "Override malloc/realloc/free (DANGEROUS)"    OFF)
option(NEXUS_EXIT_CRASH                  "Provide exit_crash()"                         OFF)
option(NEXUS_OVERRIDE_STDLIB_EXIT        "Override exit() with exit_crash (DANGEROUS)" OFF)
option(NEXUS_ENABLE_LEGACY_SHORT_ALIASES "Expose uint/boolean/TRUE/FALSE aliases"      OFF)

# Build style
option(BUILD_SHARED_LIBS "Build shared instead of static" OFF)
option(nexus_BUILD_APP   "Build demo console app (src/main.c)" ON)
option(nexus_BUILD_TESTS "Build tests in /tests"               ON)

include(GNUInstallDirs)

# -------------------- Dependencies (discover & select) --------------------
# Nexus currently has no external deps; keep placeholders for consistency.
set(NEXUS_PUBLIC_DEPS  "")
set(NEXUS_PRIVATE_DEPS "")

# If you add deps later, either:
#  - find_package(foo CONFIG REQUIRED)  (installed usage)
#  - or add_subdirectory(external/foo)  (bundled usage)
# and then:
#  list(APPEND NEXUS_PUBLIC_DEPS  foo::foo)   # if exposed in public headers
#  list(APPEND NEXUS_PRIVATE_DEPS foo::foo)   # if used only in .c files

# -------------------------------------------------------------------------

# ===== Sources (recursively, excluding app entry) =====
file(GLOB_RECURSE NEXUS_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)
# Exclude optional app entry from the library if present
list(FILTER NEXUS_SOURCES EXCLUDE REGEX "/main\\.c$")

# ===== Generated config header (namespaced path) =====
set(NEXUS_CFG_TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/include/nexus/nexus_build_config.h.in)
set(NEXUS_CFG_OUTDIR   ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${NEXUS_CFG_OUTDIR}/nexus)

# Snapshot numbers (1/0) for the template (these reflect CMake *options* at configure time)
set(NEXUS_DOUBLE_PRECISION_NUM            0)
set(NEXUS_MEMORY_DEBUG_NUM                0)
set(NEXUS_OVERRIDE_STDLIB_ALLOC_NUM       0)
set(NEXUS_EXIT_CRASH_NUM                  0)
set(NEXUS_OVERRIDE_STDLIB_EXIT_NUM        0)
set(NEXUS_ENABLE_LEGACY_SHORT_ALIASES_NUM 0)
if(NEXUS_DOUBLE_PRECISION)
  set(NEXUS_DOUBLE_PRECISION_NUM 1)
endif()
if(NEXUS_MEMORY_DEBUG)
  set(NEXUS_MEMORY_DEBUG_NUM 1)
endif()
if(NEXUS_OVERRIDE_STDLIB_ALLOC)
  set(NEXUS_OVERRIDE_STDLIB_ALLOC_NUM 1)
endif()
if(NEXUS_EXIT_CRASH)
  set(NEXUS_EXIT_CRASH_NUM 1)
endif()
if(NEXUS_OVERRIDE_STDLIB_EXIT)
  set(NEXUS_OVERRIDE_STDLIB_EXIT_NUM 1)
endif()
if(NEXUS_ENABLE_LEGACY_SHORT_ALIASES)
  set(NEXUS_ENABLE_LEGACY_SHORT_ALIASES_NUM 1)
endif()

configure_file(
        ${NEXUS_CFG_TEMPLATE}
        ${NEXUS_CFG_OUTDIR}/nexus/nexus_build_config.h
        @ONLY
)

# ===== Library =====
add_library(nexus ${NEXUS_SOURCES})
add_library(nexus::nexus ALIAS nexus)

set_target_properties(nexus PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        VERSION   ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        OUTPUT_NAME nexus
)

# Public headers for build & install trees (+ generated header dir)
target_include_directories(nexus
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Version/config macros (always after add_library)
target_compile_definitions(nexus PUBLIC
        NEXUS_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        NEXUS_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        NEXUS_VERSION_PATCH=${PROJECT_VERSION_PATCH}
        NEXUS_CFG_PP_BUILD_CONFIG="$<CONFIG>"
)

# Feature toggles to preprocessor (apply after add_library)
if(NEXUS_DOUBLE_PRECISION)
  target_compile_definitions(nexus PUBLIC NEXUS_DOUBLE_PRECISION)
endif()

# Memory debug: explicit ON wins; otherwise, auto-enable in Debug
if(NEXUS_MEMORY_DEBUG)
  target_compile_definitions(nexus PUBLIC NEXUS_MEMORY_DEBUG)
else()
  target_compile_definitions(nexus PUBLIC $<$<CONFIG:Debug>:NEXUS_MEMORY_DEBUG>)
endif()

if(NEXUS_ENABLE_LEGACY_SHORT_ALIASES)
  target_compile_definitions(nexus PUBLIC NEXUS_ENABLE_LEGACY_SHORT_ALIASES)
endif()

# Dangerous overrides stay PRIVATE (don’t leak to consumers by default)
if(NEXUS_OVERRIDE_STDLIB_ALLOC)
  target_compile_definitions(nexus PRIVATE NEXUS_OVERRIDE_STDLIB_ALLOC)
endif()
if(NEXUS_EXIT_CRASH)
  target_compile_definitions(nexus PRIVATE NEXUS_EXIT_CRASH)
  if(NEXUS_OVERRIDE_STDLIB_EXIT)
    target_compile_definitions(nexus PRIVATE NEXUS_OVERRIDE_STDLIB_EXIT)
  endif()
endif()

# Windows DLL import/export hygiene
if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(nexus PRIVATE NEXUS_BUILD_DLL)
endif()

# ---- Apply dependency link interfaces (safe with empty lists) ----
target_link_libraries(nexus
        PUBLIC  ${NEXUS_PUBLIC_DEPS}
        PRIVATE ${NEXUS_PRIVATE_DEPS}
)

# Warnings
if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
  target_compile_options(nexus PRIVATE /W4)
elseif (MSVC)
  target_compile_options(nexus PRIVATE /W4)
endif()

# ===== Demo app (optional) =====
if(nexus_BUILD_APP)
  add_executable(nexus_app src/main.c)
  target_link_libraries(nexus_app PRIVATE nexus::nexus)
  # If you want app to see the dangerous overrides too:
  if(NEXUS_OVERRIDE_STDLIB_ALLOC)
    target_compile_definitions(nexus_app PRIVATE NEXUS_OVERRIDE_STDLIB_ALLOC)
  endif()
  if(NEXUS_EXIT_CRASH)
    target_compile_definitions(nexus_app PRIVATE NEXUS_EXIT_CRASH)
    if(NEXUS_OVERRIDE_STDLIB_EXIT)
      target_compile_definitions(nexus_app PRIVATE NEXUS_OVERRIDE_STDLIB_EXIT)
    endif()
  endif()
endif()

# ===== Tests (optional) =====
if(nexus_BUILD_TESTS)
  enable_testing()
  add_executable(nexus_tests tests/entry.c)
  target_link_libraries(nexus_tests PRIVATE nexus::nexus)
  add_test(NAME nexus.basic COMMAND nexus_tests)
endif()

# ===== Install & export =====
include(CMakePackageConfigHelpers)

install(TARGETS nexus
        EXPORT nexusTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${NEXUS_CFG_OUTDIR}/nexus/nexus_build_config.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/nexus
)

configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/nexus-config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/nexus-config.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/nexus
)

write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/nexus-config-version.cmake
        VERSION ${PROJECT_VERSION}
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

# Build-tree export for add_subdirectory consumers
export(EXPORT nexusTargets
        NAMESPACE nexus::
        FILE ${CMAKE_CURRENT_BINARY_DIR}/nexusTargets.cmake
)

# ===== Diagnostics =====
message(STATUS "NEXUS_VERSION                     = ${PROJECT_VERSION}")
message(STATUS "NEXUS_DOUBLE_PRECISION            = ${NEXUS_DOUBLE_PRECISION}")
message(STATUS "NEXUS_MEMORY_DEBUG                = ${NEXUS_MEMORY_DEBUG} (auto-ON in Debug)")
message(STATUS "NEXUS_OVERRIDE_STDLIB_ALLOC       = ${NEXUS_OVERRIDE_STDLIB_ALLOC}")
message(STATUS "NEXUS_EXIT_CRASH                  = ${NEXUS_EXIT_CRASH}")
message(STATUS "NEXUS_OVERRIDE_STDLIB_EXIT        = ${NEXUS_OVERRIDE_STDLIB_EXIT}")
message(STATUS "NEXUS_ENABLE_LEGACY_SHORT_ALIASES = ${NEXUS_ENABLE_LEGACY_SHORT_ALIASES}")
