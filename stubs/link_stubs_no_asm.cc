// link_stubs.cc - Stub implementations for symbols that dex2oat
// references but doesn't actually call during AOT compilation.
// These prevent null-pointer crashes from --unresolved-symbols=ignore-in-object-files.

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <stdarg.h>

extern "C" {

// === liblog ===
// Minimal implementation that routes all log output to stderr.

struct __android_log_message {
    size_t struct_size;
    int32_t buffer_id;
    int32_t priority;
    const char* tag;
    const char* file;
    uint32_t line;
    const char* message;
};

typedef void (*__android_logger_function)(const struct __android_log_message* log_message);
typedef void (*__android_aborter_function)(const char* abort_message);

static __android_logger_function g_logger = nullptr;
static __android_aborter_function g_aborter = nullptr;
static int g_min_priority = 0;  // VERBOSE=0, DEBUG=1, INFO=2, WARN=3, ERROR=4, FATAL=5
static const char* g_default_tag = "dex2oat";

static void default_stderr_logger(const struct __android_log_message* msg) {
    fprintf(stderr, "%s: %s\n", msg->tag ? msg->tag : g_default_tag, msg->message ? msg->message : "");
    fflush(stderr);
}

static void default_aborter(const char* msg) {
    fprintf(stderr, "ABORT: %s\n", msg);
    fflush(stderr);
    abort();
}

int __android_log_buf_print(int, int priority, const char* tag, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "%s: ", tag ? tag : g_default_tag);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    fflush(stderr);
    return 0;
}
int __android_log_print(int priority, const char* tag, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "%s: ", tag ? tag : g_default_tag);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    fflush(stderr);
    return 0;
}
int __android_log_is_loggable(int priority, const char*, int) { return priority >= g_min_priority; }
void __android_log_set_logger(__android_logger_function logger) { g_logger = logger; }
void __android_log_write_log_message(struct __android_log_message* msg) {
    if (g_logger) {
        g_logger(msg);
    } else {
        default_stderr_logger(msg);
    }
}
void __android_log_logd_logger(const struct __android_log_message* msg) {
    default_stderr_logger(msg);
}
void __android_log_stderr_logger(const struct __android_log_message* msg) {
    default_stderr_logger(msg);
}
void __android_log_set_aborter(__android_aborter_function aborter) { g_aborter = aborter; }
void __android_log_call_aborter(const char* msg) {
    if (g_aborter) g_aborter(msg); else default_aborter(msg);
}
void __android_log_default_aborter(const char* msg) { default_aborter(msg); }
int __android_log_set_minimum_priority(int p) { int old = g_min_priority; g_min_priority = p; return old; }
int __android_log_get_minimum_priority() { return g_min_priority; }
void __android_log_set_default_tag(const char* tag) { if (tag) g_default_tag = tag; }

// === LZ4 ===
int LZ4_compressBound(int inputSize) { return inputSize + inputSize / 255 + 16; }
int LZ4_compress_default(const char* src, char* dst, int srcSize, int dstCapacity) {
    if (srcSize > dstCapacity) return 0;
    memcpy(dst, src, srcSize);
    return srcSize;
}
int LZ4_compress_HC(const char* src, char* dst, int srcSize, int dstCapacity, int) {
    return LZ4_compress_default(src, dst, srcSize, dstCapacity);
}
int LZ4_decompress_safe(const char* src, char* dst, int compressedSize, int dstCapacity) {
    if (compressedSize > dstCapacity) return -1;
    memcpy(dst, src, compressedSize);
    return compressedSize;
}

// === ART palette ===
int PaletteTraceEnabled(bool* enabled) { if (enabled) *enabled = false; return 0; }
int PaletteTraceBegin(const char*) { return 0; }
int PaletteTraceEnd() { return 0; }
int PaletteTraceIntegerValue(const char*, int) { return 0; }
int PaletteSchedGetPriority(int, int* p) { if (p) *p = 0; return 0; }
int PaletteSchedSetPriority(int, int) { return 0; }

// === Native bridge (not used in dex2oat) ===
int InitializeNativeBridge(void*, const char*) { return 0; }
int LoadNativeBridge(const char*, void*) { return 0; }
int NativeBridgeInitialized() { return 0; }
int NativeBridgeGetVersion() { return 0; }
void* NativeBridgeGetTrampoline(void*, const char*, const char*, int) { return nullptr; }
void* NativeBridgeGetSignalHandler(int) { return nullptr; }
void PreInitializeNativeBridge(const char*, const char*) {}
void PreZygoteForkNativeBridge() {}
void UnloadNativeBridge() {}

// === Native loader (not used in dex2oat) ===
int InitializeNativeLoader() { return 0; }
int ResetNativeLoader() { return 0; }
void* OpenNativeLibrary(void*, const char*, void*, void*, void*, void*, void*) { return nullptr; }
int CloseNativeLibrary(void*, void*, void*) { return 0; }
void NativeLoaderFreeErrorMessage(char*) {}

// === LZMA (used for compressed OAT, not critical for basic compilation) ===
void Lzma2EncProps_Init(void*) {}
void Lzma2EncProps_Normalize(void*) {}
int Xz_Encode(void*, void*, void*, void*) { return -1; }
void XzProps_Init(void*) {}
void XzUnpacker_Construct(void*, void*) {}
void XzUnpacker_Free(void*) {}
int XzUnpacker_Code(void*, void*, void*, void*, void*, int, void*) { return -1; }
int XzUnpacker_IsStreamWasFinished(void*) { return 1; }
void CrcGenerateTable() {}
void Crc64GenerateTable() {}

// === SHA1 (used for checksums, stub for now) ===
void SHA1_Init(void*) {}
void SHA1_Update(void* ctx, const void* data, unsigned long len) {}
void SHA1_Final(unsigned char* md, void* ctx) { if (md) memset(md, 0, 20); }

// __memcmp16 provided by memcmp16_x86_64.S

// === Palette crash stacks ===
int PaletteWriteCrashThreadStacks(const char*, unsigned long) { return 0; }

}  // extern "C"

// === C++ stubs in art namespace ===
// Only for symbols that can't be compiled from real AOSP sources.
#include <ostream>

// === BacktraceMap (outside extern "C") ===
class BacktraceMap {
public:
    static BacktraceMap* Create(int, bool);
};
BacktraceMap* BacktraceMap::Create(int, bool) { return nullptr; }

namespace art {

// DumpNativeStack - needs libbacktrace
class ArtMethod;
void DumpNativeStack(std::ostream&, int, BacktraceMap*, const char*, ArtMethod*, void*, bool) {}

// BacktraceCollector - needs libunwindstack
class BacktraceCollector {
public:
  void Collect();
};
void BacktraceCollector::Collect() {}

// SafeCopy - needs sys/ucontext
size_t SafeCopy(void* dst, const void* src, size_t len) {
    memcpy(dst, src, len);
    return len;
}

// hprof - not needed for dex2oat
namespace hprof {
  void DumpHeap(const char*, int, bool) {}
}

}  // namespace art
