#include <stdio.h>
#include "nexus/nexus.h"

/* Simple example API impl */
int nexus_add(int a, int b) {
    return a + b;
}

const char* nexus_version_string(void) {
    /* Keep this tiny and compile-time only */
    return "0.0.0";
}
