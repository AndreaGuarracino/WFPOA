#include <stdio.h>

#include "utils.h"

void err_fatal(const char *header, const char *fmt, ...)
{
    fprintf(stderr, "[%s]\n", header);
    exit(EXIT_FAILURE);
}

void _err_fatal_simple(const char *func, const char *msg)
{
    fprintf(stderr, "[%s] %s\n", func, msg);
    exit(EXIT_FAILURE);
}

FILE *err_xopen_core(const char *func, const char *fn, const char *mode)
{
    FILE *fp = 0;
    if (strcmp(fn, "-") == 0)
        return (strstr(mode, "r"))? stdin : stdout;
    if ((fp = fopen(fn, mode)) == 0) {
        err_fatal(func, "fail to open file '%s' : %s", fn, strerror(errno));
    }
    return fp;
}


int err_fclose(FILE *stream)
{
    int ret = fclose(stream);
    if (ret != 0) _err_fatal_simple("fclose", strerror(errno));
    return ret;
}
