/* Host-only storage guard. Rendering/assertions remain enabled; PPM persistence
 * is not tested. Scoped by the runner to direct gui_host children only. */
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
static const char *destination(const char *p, const char *mode) {
    size_t n = strlen(p);
    return strchr(mode, 'w') && n >= 4 && !strcmp(p+n-4, ".ppm") ? "/dev/null" : p;
}
FILE *fopen(const char *p, const char *mode) {
    FILE *(*real)(const char *, const char *) = dlsym(RTLD_NEXT, "fopen");
    return real(destination(p, mode), mode);
}
FILE *fopen64(const char *p, const char *mode) {
    FILE *(*real)(const char *, const char *) = dlsym(RTLD_NEXT, "fopen64");
    return real(destination(p, mode), mode);
}
