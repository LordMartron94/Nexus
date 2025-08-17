/* tests/entry.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nexus/nexus.h"
#include "nexus/nexus_build_config.h"

/* Pretty-print helpers */
static const char* yesno(int v) { return v ? "ON " : "OFF"; }

static void print_rule(const char* name, int pp_val, int cmake_val) {
    const char* status = (pp_val == cmake_val) ? "OK " : "DIFF";
    printf("  %-30s  PP=%-3s  CMAKE=%-3s  [%s]\n", name, yesno(pp_val), yesno(cmake_val), status);
}

static void show_configuration(void) {
    puts("=== NEXUS CONFIGURATION ===");

    /* Versions */
    printf("Version (macros): %d.%d.%d\n",
           NEXUS_CFG_VERSION_MAJOR,
           NEXUS_CFG_VERSION_MINOR,
           NEXUS_CFG_VERSION_PATCH);

    /* Version string from runtime API */
    {
        const char* v = nexus_version_string();
        printf("Version (API)   : %s\n", v ? v : "<NULL>");
    }

    /* Build type strings:
       - NEXUS_CFG_PP_BUILD_CONFIG is provided via target_compile_definitions(... "$<CONFIG>") if you enabled it.
       - NEXUS_CFG_CMAKE_BUILD_TYPE is stamped by CMake at configure time and is empty on multi-config generators. */
    printf("Build config    : PP=\"%s\"  |  CMAKE_SNAPSHOT=\"%s\"\n",
           NEXUS_CFG_PP_BUILD_CONFIG, NEXUS_CFG_CMAKE_BUILD_TYPE);

    puts("\n--- Feature toggles (PP vs CMake snapshot) ---");
    print_rule("DOUBLE_PRECISION",
               NEXUS_CFG_PP_DOUBLE_PRECISION,
               (int){NEXUS_CFG_CMAKE_DOUBLE_PRECISION});

    print_rule("MEMORY_DEBUG",
               NEXUS_CFG_PP_MEMORY_DEBUG,
               (int){NEXUS_CFG_CMAKE_MEMORY_DEBUG});

    print_rule("OVERRIDE_ALLOC",
               NEXUS_CFG_PP_OVERRIDE_ALLOC,
               (int){NEXUS_CFG_CMAKE_OVERRIDE_ALLOC});

    print_rule("EXIT_CRASH",
               NEXUS_CFG_PP_EXIT_CRASH,
               (int){NEXUS_CFG_CMAKE_EXIT_CRASH});

    print_rule("OVERRIDE_EXIT",
               NEXUS_CFG_PP_OVERRIDE_EXIT,
               (int){NEXUS_CFG_CMAKE_OVERRIDE_EXIT});

    print_rule("LEGACY_ALIASES",
               NEXUS_CFG_PP_LEGACY_ALIASES,
               (int){NEXUS_CFG_CMAKE_LEGACY_ALIASES});

    puts("=== END CONFIGURATION ===\n");
}

static int run_basic_api_tests(void) {
    int ok = 1;

    if (nexus_add(2, 3) != 5) {
        fprintf(stderr, "add(2,3) != 5\n");
        ok = 0;
    }
    if (nexus_version_string() == NULL) {
        fprintf(stderr, "version_string() returned NULL\n");
        ok = 0;
    }
    return ok;
}

static void exercise_memory_debug(void) {
#if defined(NEXUS_MEMORY_DEBUG)
    puts("[memdbg] NEXUS_MEMORY_DEBUG is ON -- exercising debug allocator.");

    /* If your implementation needs locks, pass function pointers; we keep it NULL for simplicity. */
    nexus_debug_memory_init(NULL, NULL, NULL);

    /* Allocate, touch, and free to verify the hooks are wired. */
    {
        const size_t n = 128;
        unsigned char* p = NEXUS_ALLOC(n);
        if (!p) {
            fprintf(stderr, "[memdbg] NEXUS_ALLOC(%lu) returned NULL\n", (unsigned long)n);
        } else {
            /* Touch within bounds */
            for (size_t i = 0; i < n; ++i) p[i] = (unsigned char)i;
            p = (unsigned char*)NEXUS_REALLOC(p, n * 2);
            if (!p) {
                fprintf(stderr, "[memdbg] NEXUS_REALLOC failed\n");
            } else {
                /* Touch the extended part */
                for (size_t i = 0; i < n * 2; ++i) (void)p[i];
            }
            NEXUS_FREE(p);
        }
    }

    /* Optional: demonstrate outstanding allocation reporting (allocate and free) */
    {
        void* leak = NEXUS_ALLOC(64);
        if (!leak) {
            fprintf(stderr, "[memdbg] NEXUS_ALLOC(64) returned NULL\n");
        }
        /* Print current state (implementation-defined formatting) */
        puts("[memdbg] after temp alloc:");
        nexus_debug_mem_print(0);

        /* Now free to avoid real leaks in tests */
        NEXUS_FREE(leak);

        puts("[memdbg] after free:");
        nexus_debug_mem_print(0);
    }

    /* Check guard/consistency status if provided. Note: this typically reports guard/corruption, not mere leaks. */
    {
        NEXUS_BOOL bad = nexus_debug_memory();
        printf("[memdbg] guard/corruption check: %s\n", bad ? "ISSUES DETECTED" : "OK");
    }

    /* Reset internal tables (optional, depends on your implementation) */
    nexus_debug_mem_reset();

#else
    puts("[memdbg] NEXUS_MEMORY_DEBUG is OFF — skipping allocator tests.");
#endif
}

int main(void) {
    show_configuration();

    if (!run_basic_api_tests()) {
        return EXIT_FAILURE;
    }

    exercise_memory_debug();

    puts("basic test passed");
    return EXIT_SUCCESS;
}
