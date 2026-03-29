#ifndef ANDROID_LOG_H
#define ANDROID_LOG_H
#include <stdio.h>
#include <stdarg.h>
enum { ANDROID_LOG_DEBUG=3, ANDROID_LOG_INFO=4, ANDROID_LOG_WARN=5, ANDROID_LOG_ERROR=6 };
static inline int __android_log_print(int prio, const char* tag, const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    printf("[%s] ", tag); vprintf(fmt, ap); printf("\n"); fflush(stdout);
    va_end(ap); return 0;
}
#endif
