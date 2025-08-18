#include <stdio.h>
#include "nexus/nexus.h"

int main(void) {
    printf("[nexus] Hello from demo app!\n");
    printf("[nexus] version = %s\n", nexus_version_string());
    return 0;
}
