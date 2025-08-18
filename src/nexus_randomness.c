/* Created by LordMartron on 19/08/2025. */

#include <nexus/nexus.h>
#include <time.h>
#include <stddef.h>

nexus_u32 nexus_randomness_integer_random(nexus_u32 seed)
{
    seed = (seed << 13) ^ seed;
    return ((seed * (seed * seed * 15731 * 789221) + 1376312589) & 0x7fffffff);
}

nexus_u32 nexus_randomness_seed_per_run(const void *token) {
    size_t token_size = (size_t)token;
    nexus_u32 seed = token_size;
    if (sizeof(size_t) > 4) seed ^= (nexus_u32)(token_size >> 32);

    /* mix in time-based entropy */
    seed ^= (nexus_u32)time(NULL);  /* seconds since epoch */
    seed ^= (nexus_u32)clock();     /* CPU ticks since program start */

    /* finish with a cheap mixer */
    seed ^= seed >> 16; seed *= 0x7FEB352Du;
    seed ^= seed >> 15; seed *= 0x846CA68Bu;
    seed ^= seed >> 16;
    return seed;
}