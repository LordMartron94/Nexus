#ifndef NEXUS_H
#define NEXUS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Version macros (optional) */
#ifndef NEXUS_VERSION_MAJOR
#define NEXUS_VERSION_MAJOR 0
#endif
#ifndef NEXUS_VERSION_MINOR
#define NEXUS_VERSION_MINOR 0
#endif
#ifndef NEXUS_VERSION_PATCH
#define NEXUS_VERSION_PATCH 0
#endif

/* Public API */
int nexus_add(int a, int b);
const char* nexus_version_string(void);

#ifdef __cplusplus
}
#endif

#endif /* NEXUS_H */
