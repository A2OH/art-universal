// Stub for android/log.h - not on host
#ifndef STUB_LOG_LOG_H
#define STUB_LOG_LOG_H

#include <stdio.h>

typedef enum android_LogPriority {
  ANDROID_LOG_UNKNOWN = 0,
  ANDROID_LOG_DEFAULT,
  ANDROID_LOG_VERBOSE,
  ANDROID_LOG_DEBUG,
  ANDROID_LOG_INFO,
  ANDROID_LOG_WARN,
  ANDROID_LOG_ERROR,
  ANDROID_LOG_FATAL,
  ANDROID_LOG_SILENT,
} android_LogPriority;

static inline int __android_log_print(int prio, const char* tag, const char* fmt, ...) {
  return 0;
}

static inline int __android_log_write(int prio, const char* tag, const char* msg) {
  return 0;
}

static inline int __android_log_buf_write(int bufID, int prio, const char* tag, const char* msg) {
  return 0;
}

#ifndef LOG_TAG
#define LOG_TAG nullptr
#endif

#define android_errorWriteLog(tag, subTag) 0

#endif
