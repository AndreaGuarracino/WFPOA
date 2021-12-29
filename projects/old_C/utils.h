#ifndef WFPOA_UTILS_H
#define WFPOA_UTILS_H

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#ifdef __GNUC__
// Tell GCC to validate printf format string and args
#define ATTRIBUTE(list) __attribute__ (list)
#else
#define ATTRIBUTE(list)
#endif

void err_fatal(const char *header, const char *fmt, ...) ATTRIBUTE((noreturn));
void _err_fatal_simple(const char *func, const char *msg) ATTRIBUTE((noreturn));

FILE *err_xopen_core(const char *func, const char *fn, const char *mode);

int err_fclose(FILE *stream);

#endif //WFPOA_UTILS_H
