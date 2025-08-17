set(CMAKE_C_COMPILER   "C:/CLang/bin/clang-cl.exe" CACHE FILEPATH "" FORCE)
set(CMAKE_CXX_COMPILER "C:/CLang/bin/clang-cl.exe" CACHE FILEPATH "" FORCE)

set(CMAKE_LINKER       "C:/CLang/bin/lld-link.exe"  CACHE FILEPATH "" FORCE)
set(CMAKE_AR           "C:/CLang/bin/llvm-lib.exe"  CACHE FILEPATH "" FORCE)
set(CMAKE_RANLIB       ""                           CACHE STRING  "" FORCE)
set(CMAKE_RC_COMPILER  "C:/CLang/bin/llvm-rc.exe"   CACHE FILEPATH "" FORCE)
set(CMAKE_MT           "C:/CLang/bin/llvm-mt.exe"   CACHE FILEPATH "" FORCE)

# Propagate into try-compile projects
set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
        CMAKE_LINKER CMAKE_AR CMAKE_RANLIB CMAKE_RC_COMPILER CMAKE_MT)