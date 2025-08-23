/* Created by LordMartron on 22/08/2025. */


#ifndef NEXUS_INTERNAL_H
#define NEXUS_INTERNAL_H
#include <nexus/nexus.h>
#include <stdio.h>

typedef struct NEXUS_FILE_INFORMATION {
  FILE *stream;
  nexus_u64 index;
} NEXUS_FILE_INFORMATION;


#endif /* NEXUS_INTERNAL_H */
