/* nexus.h — central primitives, versioning, and opt-in debug hooks (C89-compatible) */
#ifndef NEXUS_H
#define NEXUS_H

/* ===== Standard headers ===== */
#include <stddef.h>  /* size_t, NULL */
#include <stdlib.h>  /* malloc, realloc, free, exit */

/* ===== C++ interop ===== */
#ifdef __cplusplus
#  define NEXUS_EXTERN_C_BEGIN extern "C" {
#  define NEXUS_EXTERN_C_END   }
#else
#  define NEXUS_EXTERN_C_BEGIN
#  define NEXUS_EXTERN_C_END
#endif

/* ===== Config knobs (set via compiler flags ideally) =====
   -DNEXUS_DOUBLE_PRECISION           : float_real == double
   -DNEXUS_MEMORY_DEBUG               : enable debug alloc API
   -DNEXUS_OVERRIDE_STDLIB_ALLOC      : (with MEMORY_DEBUG) macro-replace malloc/realloc/free
   -DNEXUS_EXIT_CRASH                 : provide exit_crash() and optional exit() override
   -DNEXUS_OVERRIDE_STDLIB_EXIT       : (with EXIT_CRASH) macro-replace exit()
   -DNEXUS_ENABLE_LEGACY_SHORT_ALIASES: expose 'uint', 'boolean', TRUE/FALSE
*/

/* ===== Versioning ===== */
#ifndef NEXUS_VERSION_MAJOR
#  define NEXUS_VERSION_MAJOR 0
#endif
#ifndef NEXUS_VERSION_MINOR
#  define NEXUS_VERSION_MINOR 0
#endif
#ifndef NEXUS_VERSION_PATCH
#  define NEXUS_VERSION_PATCH 0
#endif

/* ===== Basic booleans (C89) ===== */
typedef unsigned char NEXUS_BOOL;
#ifndef NEXUS_TRUE
#  define NEXUS_TRUE  ((NEXUS_BOOL)1)
#endif
#ifndef NEXUS_FALSE
#  define NEXUS_FALSE ((NEXUS_BOOL)0)
#endif

/* ===== Sized integers (prefer stdint when available) ===== */
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
  #include <stdint.h>
  typedef int8_t    nexus_i8;   typedef uint8_t  nexus_u8;
  typedef int16_t   nexus_i16;  typedef uint16_t nexus_u16;
  typedef int32_t   nexus_i32;  typedef uint32_t nexus_u32;
  typedef int64_t   nexus_i64;  typedef uint64_t nexus_u64;
#else
  typedef signed   char      nexus_i8;   typedef unsigned char      nexus_u8;
  typedef signed   short     nexus_i16;  typedef unsigned short     nexus_u16;
  typedef signed   int       nexus_i32;  typedef unsigned int       nexus_u32;
  #if defined(_MSC_VER)
    typedef          __int64 nexus_i64;  typedef unsigned __int64   nexus_u64;
  #elif defined(__GNUC__) || defined(__clang__)
    typedef long long            nexus_i64;
    typedef unsigned long long   nexus_u64;
  #endif
#endif

/* Optional legacy aliases (opt-in) */
#ifdef NEXUS_ENABLE_LEGACY_SHORT_ALIASES
  typedef nexus_u32 uint;
  typedef NEXUS_BOOL boolean;
  #ifndef TRUE
  #define TRUE 1
  #endif
  #ifndef FALSE
  #define FALSE 0
  #endif
#endif

/* ===== Real precision switch ===== */
#ifdef NEXUS_DOUBLE_PRECISION
  typedef double float_real;
#else
  typedef float  float_real;
#endif

/* ===== Public API visibility (no-ops by default) ===== */
#ifndef NEXUS_API
#  if defined(_WIN32) && defined(NEXUS_BUILD_DLL)
#    define NEXUS_API __declspec(dllexport)
#  elif defined(_WIN32) && defined(NEXUS_USE_DLL)
#    define NEXUS_API __declspec(dllimport)
#  else
#    define NEXUS_API
#  endif
#endif

NEXUS_EXTERN_C_BEGIN

/* ===== Debug memory API (opt-in) ===== */
#ifdef NEXUS_MEMORY_DEBUG
void      nexus_debug_memory_init(void (*lock)(void*), void (*unlock)(void*), void *mutex);
void     *nexus_debug_mem_malloc(size_t size, const char *file, unsigned line);
void     *nexus_debug_mem_realloc(void *ptr, size_t size, const char *file, unsigned line);
void      nexus_debug_mem_free(void *ptr);
void      nexus_debug_mem_print(unsigned min_allocs);
void      nexus_debug_mem_reset(void);
NEXUS_BOOL nexus_debug_memory(void); /* NEXUS_TRUE if any guard error found */

#  define NEXUS_ALLOC(n)      nexus_debug_mem_malloc((n), __FILE__, __LINE__)
#  define NEXUS_REALLOC(p, n) nexus_debug_mem_realloc((p), (n), __FILE__, __LINE__)
#  define NEXUS_FREE(p)       nexus_debug_mem_free((p))

#  ifdef NEXUS_OVERRIDE_STDLIB_ALLOC
#    undef  malloc
#    undef  realloc
#    undef  free
#    define malloc(n)      nexus_debug_mem_malloc((n), __FILE__, __LINE__)
#    define realloc(p, n)  nexus_debug_mem_realloc((p), (n), __FILE__, __LINE__)
#    define free(p)        nexus_debug_mem_free((p))
#  endif
#else
#  define NEXUS_ALLOC(n)      malloc((n))
#  define NEXUS_REALLOC(p, n) realloc((p), (n))
#  define NEXUS_FREE(p)       free((p))
#endif

/* ===== Exit crash (opt-in) ===== */
#ifdef NEXUS_EXIT_CRASH
void exit_crash(unsigned code);
#  ifdef NEXUS_OVERRIDE_STDLIB_EXIT
#    undef exit
#    define exit(code) exit_crash((unsigned)(code))
#  endif
#endif

/* ===== Public API surface ===== */
/* Returns a static buffer or constant string describing MAJOR.MINOR.PATCH */
NEXUS_API const char* nexus_version_string(void);

nexus_u32 nexus_randomness_integer_random(nexus_u32 seed);

nexus_u32 nexus_randomness_seed_per_run(const void *token);

NEXUS_EXTERN_C_END
#endif /* NEXUS_H */
