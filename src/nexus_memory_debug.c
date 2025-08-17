/* nexus_debug_alloc.c — C89-compatible debug allocator for nexus.h */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <nexus/nexus.h>

/* If the header asked to override stdlib symbols for general code,
   we must call the real CRT here to avoid recursion. */
#ifdef NEXUS_OVERRIDE_STDLIB_ALLOC
#  undef malloc
#  undef realloc
#  undef free
#endif
#ifdef NEXUS_OVERRIDE_STDLIB_EXIT
#  undef exit
#endif

/* ------------------------------------------------------------------ */
/* Config                                                              */
/* ------------------------------------------------------------------ */
#define NEXUS_MEMORY_OVER_ALLOC   32
#define NEXUS_MEMORY_MAGIC_NUMBER 132

/* ------------------------------------------------------------------ */
/* Types & globals                                                     */
/* ------------------------------------------------------------------ */
typedef struct {
    size_t size;     /* requested size (no guards) */
    void  *buf;      /* start of the raw allocation (guarded) */
} NexusAllocBuf;

typedef struct {
    unsigned       line;
    char           file[256];
    NexusAllocBuf *allocs;
    unsigned       alloc_count;
    unsigned       alloc_capacity;

    /* Stats */
    size_t         bytes_live;   /* sum of live payload bytes for this site */
    unsigned       alloc_total;  /* total alloc calls at this site */
    unsigned       free_total;   /* total frees at this site */
} NexusAllocLine;

static NexusAllocLine g_lines[1024];
static unsigned       g_line_count = 0;

static void *g_mutex = NULL;
static void (*g_lock)(void *mutex)   = NULL;
static void (*g_unlock)(void *mutex) = NULL;

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

static void nexus__lock(void)
{
    if (g_lock && g_mutex) g_lock(g_mutex);
}
static void nexus__unlock(void)
{
    if (g_unlock && g_mutex) g_unlock(g_mutex);
}

/* Return index of (file,line) entry, or g_line_count if not found. */
static unsigned nexus__find_site(const char *file, unsigned line)
{
    unsigned i, j;
    for (i = 0; i < g_line_count; ++i) {
        if (g_lines[i].line == line) {
            for (j = 0; g_lines[i].file[j] == file[j] && file[j] != '\0'; ++j) {
                /* compare */
            }
            if (g_lines[i].file[j] == file[j]) {
                return i;
            }
        }
    }
    return g_line_count;
}

static void nexus__ensure_capacity(NexusAllocLine *site)
{
    if (site->alloc_count == site->alloc_capacity) {
        unsigned new_cap = site->alloc_capacity ? (site->alloc_capacity + 1024u) : 256u;
        NexusAllocBuf *p = realloc(site->allocs, (size_t)new_cap * sizeof *p);
        if (!p) {
            /* Out of memory while growing the bookkeeping — bail hard. */
            fprintf(stderr, "MEM ERROR: realloc failed while growing bookkeeping\n");
            exit(1);
        }
        site->allocs = p;
        site->alloc_capacity = new_cap;
    }
}

static void nexus__site_add(const char *file, unsigned line, void *ptr, size_t size)
{
    unsigned i, j;

    /* Tail-guard the allocation with the magic byte. */
    for (i = 0; i < NEXUS_MEMORY_OVER_ALLOC; ++i) {
        ((nexus_u8*)ptr)[size + i] = (nexus_u8)NEXUS_MEMORY_MAGIC_NUMBER;
    }

    i = nexus__find_site(file, line);
    if (i < g_line_count) {
        NexusAllocLine *s = &g_lines[i];
        nexus__ensure_capacity(s);
        s->allocs[s->alloc_count].size = size;
        s->allocs[s->alloc_count].buf  = ptr;
        s->alloc_count += 1;
        s->bytes_live  += size;
        s->alloc_total += 1;
        return;
    }

    /* New site */
    if (g_line_count < (sizeof g_lines / sizeof g_lines[0])) {
        NexusAllocLine *s = &g_lines[g_line_count];
        s->line = line;
        for (j = 0; j < 255 && file[j] != '\0'; ++j) s->file[j] = file[j];
        s->file[j] = '\0';
        s->allocs = NULL;
        s->alloc_count = 0u;
        s->alloc_capacity = 0u;
        s->bytes_live = 0u;
        s->alloc_total = 0u;
        s->free_total  = 0u;

        nexus__ensure_capacity(s);
        s->allocs[0].size = size;
        s->allocs[0].buf  = ptr;
        s->alloc_count = 1u;
        s->bytes_live  = size;
        s->alloc_total = 1u;

        g_line_count += 1u;
    }
}

/* Remove a tracked allocation; returns NEXUS_TRUE if found, sets *out_size. */
static NEXUS_BOOL nexus__site_remove(void *ptr, size_t *out_size)
{
    unsigned i, j, k;
    for (i = 0; i < g_line_count; ++i) {
        NexusAllocLine *s = &g_lines[i];
        for (j = 0; j < s->alloc_count; ++j) {
            if (s->allocs[j].buf == ptr) {
                /* Check guard band */
                for (k = 0; k < NEXUS_MEMORY_OVER_ALLOC; ++k) {
                    if (((nexus_u8*)ptr)[s->allocs[j].size + k] != (nexus_u8)NEXUS_MEMORY_MAGIC_NUMBER) {
                        fprintf(stderr, "MEM ERROR: Overshoot at line %u in file %s\n",
                                s->line, s->file);
                        break;
                    }
                }
                if (out_size) *out_size = s->allocs[j].size;

                /* Remove by swap-with-last */
                s->bytes_live -= s->allocs[j].size;
                s->free_total += 1u;
                s->alloc_count -= 1u;
                s->allocs[j] = s->allocs[s->alloc_count];
                return NEXUS_TRUE;
            }
        }
    }
    return NEXUS_FALSE;
}

/* ------------------------------------------------------------------ */
/* Public API (matches nexus.h)                                        */
/* ------------------------------------------------------------------ */

void nexus_debug_memory_init(void (*lock)(void *mutex),
                             void (*unlock)(void *mutex),
                             void *mutex)
{
    g_mutex  = mutex;
    g_lock   = lock;
    g_unlock = unlock;
}

NEXUS_BOOL nexus_debug_memory(void)
{
    NEXUS_BOOL any_error = NEXUS_FALSE;
    unsigned i, j, k;

    nexus__lock();
    for (i = 0; i < g_line_count; ++i) {
        const NexusAllocLine *s = &g_lines[i];
        for (j = 0; j < s->alloc_count; ++j) {
            const nexus_u8 *buf  = s->allocs[j].buf;
            size_t          size = s->allocs[j].size;
            for (k = 0; k < NEXUS_MEMORY_OVER_ALLOC; ++k) {
                if (buf[size + k] != (nexus_u8)NEXUS_MEMORY_MAGIC_NUMBER) {
                    fprintf(stderr, "MEM ERROR: Overshoot at line %u in file %s\n",
                            s->line, s->file);
                    any_error = NEXUS_TRUE;
                    break;
                }
            }
        }
    }
    nexus__unlock();
    return any_error;
}

void *nexus_debug_mem_malloc(size_t size, const char *file, unsigned line)
{
    size_t i;
    void *p;

    nexus__lock();

    p = malloc(size + NEXUS_MEMORY_OVER_ALLOC);
    if (!p) {
        fprintf(stderr, "MEM ERROR: malloc returned NULL for %lu bytes at %s:%u\n",
                (unsigned long)size, file, line);
        nexus__unlock();
        nexus_debug_mem_print(0u);
        exit(1);
    }

    /* Fill whole region with (MAGIC+1), we’ll set the tail guards to MAGIC in add() */
    for (i = 0; i < size + NEXUS_MEMORY_OVER_ALLOC; ++i)
        ((nexus_u8*)p)[i] = (nexus_u8)(NEXUS_MEMORY_MAGIC_NUMBER + 1);

    nexus__site_add(file, line, p, size);
    nexus__unlock();
    return p;
}

void *nexus_debug_mem_realloc(void *ptr, size_t size, const char *file, unsigned line)
{
    size_t old_sz = 0u, move, i;
    void *p2;

    if (!ptr) {
        return nexus_debug_mem_malloc(size, file, line);
    }

    nexus__lock();

    if (!nexus__site_remove(ptr, &old_sz)) {
        unsigned i_site, j_ent, k_off;
        fprintf(stderr, "MEM ERROR: realloc on untracked pointer %p at %s:%u\n", ptr, file, line);
        /* Attempt to locate pointer inside a known allocation for diagnostics */
        for (i_site = 0; i_site < g_line_count; ++i_site) {
            NexusAllocLine *s = &g_lines[i_site];
            for (j_ent = 0; j_ent < s->alloc_count; ++j_ent) {
                nexus_u8 *b = s->allocs[j_ent].buf;
                for (k_off = 0; k_off < s->allocs[j_ent].size; ++k_off) {
                    if ((void*)(b + k_off) == ptr) {
                        fprintf(stderr,
                                "Note: pointer is %u bytes into an allocation from %s:%u\n",
                                k_off, s->file, s->line);
                        break;
                    }
                }
            }
        }
        nexus__unlock();
        exit(1);
    }

    move = (old_sz < size) ? old_sz : size;

    p2 = malloc(size + NEXUS_MEMORY_OVER_ALLOC);
    if (!p2) {
        fprintf(stderr, "MEM ERROR: realloc malloc failed for %lu bytes at %s:%u\n",
                (unsigned long)size, file, line);
        nexus__unlock();
        nexus_debug_mem_print(0u);
        exit(1);
    }
    for (i = 0; i < size + NEXUS_MEMORY_OVER_ALLOC; ++i)
        ((nexus_u8*)p2)[i] = (nexus_u8)(NEXUS_MEMORY_MAGIC_NUMBER + 1);

    memcpy(p2, ptr, move);
    nexus__site_add(file, line, p2, size);
    free(ptr);

    nexus__unlock();
    return p2;
}

void nexus_debug_mem_free(void *ptr)
{
    size_t sz_dummy;
    nexus__lock();
    if (!nexus__site_remove(ptr, &sz_dummy)) {
        /* Double free or foreign pointer */
        fprintf(stderr, "MEM ERROR: free on untracked pointer %p\n", ptr);
        /* Force a crash to ease debugging, matching original intent */
        { volatile unsigned *X = (unsigned*)0; *X = 0; }
    }
    free(ptr);
    nexus__unlock();
}

void nexus_debug_mem_print(unsigned min_allocs)
{
    unsigned i;

    nexus__lock();
    printf("Memory report:\n----------------------------------------------\n");
    for (i = 0; i < g_line_count; ++i) {
        const NexusAllocLine *s = &g_lines[i];
        if (s->alloc_total > min_allocs) {
            printf("%s line: %u\n", s->file, s->line);
            printf(" - Bytes live: %lu\n - Allocations: %u\n - Frees: %u\n\n",
                   (unsigned long)s->bytes_live, s->alloc_total, s->free_total);
        }
    }
    printf("----------------------------------------------\n");
    nexus__unlock();
}

nexus_u32 nexus_debug_mem_consumption(void)
{
    /* NOTE: Returns 32-bit for legacy compatibility; may truncate on huge heaps. */
    unsigned i;
    size_t sum = 0u;

    nexus__lock();
    for (i = 0; i < g_line_count; ++i) sum += g_lines[i].bytes_live;
    nexus__unlock();

    return sum;
}

void nexus_debug_mem_reset(void)
{
    unsigned i;
    nexus__lock();
    for (i = 0; i < g_line_count; ++i) {
        free(g_lines[i].allocs);
        g_lines[i].allocs = NULL;
        g_lines[i].alloc_count = 0u;
        g_lines[i].alloc_capacity = 0u;
        g_lines[i].bytes_live = 0u;
        g_lines[i].alloc_total = 0u;
        g_lines[i].free_total  = 0u;
    }
    g_line_count = 0u;
    nexus__unlock();
}

#ifdef NEXUS_EXIT_CRASH
void exit_crash(unsigned code)
{
    (void)code;
    /* Deliberately crash: null write */
    { volatile unsigned *a = (unsigned*)0; *a = 0; }
}
#endif
