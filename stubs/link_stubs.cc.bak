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

// === __memcmp16 (bionic-specific, used by String comparisons) ===
int __memcmp16(const uint16_t* s1, const uint16_t* s2, size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (s1[i] != s2[i]) return s1[i] < s2[i] ? -1 : 1;
    }
    return 0;
}

// === Palette crash stacks ===
int PaletteWriteCrashThreadStacks(const char*, unsigned long) { return 0; }

// === Assembly entrypoints (never called by dex2oat, only by running ART VM) ===
// These are normally defined in arch-specific .S files.
// dex2oat only needs their addresses as function pointers, it never calls them.

#define STUB_ENTRYPOINT(name) void name() { fprintf(stderr, "FATAL: stub entrypoint " #name " called\n"); abort(); }

// Interpreter
STUB_ENTRYPOINT(ExecuteMterpImpl)
STUB_ENTRYPOINT(ExecuteNterpImpl)
STUB_ENTRYPOINT(ExecuteSwitchImplAsm)

// artMterpAsmInstructionStart/End must be spaced exactly 256*128 = 32768 bytes apart
// for CheckMterpAsmConstants() to pass.
__attribute__((aligned(128))) char artMterpAsmInstructionStart[32768] = {};
char artMterpAsmInstructionEnd[1] = {};

// artNterpAsmInstructionStart/End - same pattern for nterp
__attribute__((aligned(128))) char artNterpAsmInstructionStart[32768] = {};
char artNterpAsmInstructionEnd[1] = {};

// Quick entrypoints
STUB_ENTRYPOINT(art_quick_do_long_jump)
STUB_ENTRYPOINT(art_quick_deliver_exception)
STUB_ENTRYPOINT(art_quick_deoptimize)
STUB_ENTRYPOINT(art_quick_deoptimize_from_compiled_code)
STUB_ENTRYPOINT(art_quick_test_suspend)
STUB_ENTRYPOINT(art_quick_resolution_trampoline)
STUB_ENTRYPOINT(art_quick_to_interpreter_bridge)
STUB_ENTRYPOINT(art_quick_generic_jni_trampoline)
STUB_ENTRYPOINT(art_quick_imt_conflict_trampoline)
STUB_ENTRYPOINT(art_quick_proxy_invoke_handler)
STUB_ENTRYPOINT(art_quick_osr_stub)
STUB_ENTRYPOINT(art_quick_compile_optimized)
STUB_ENTRYPOINT(art_quick_invoke_stub_internal)
STUB_ENTRYPOINT(art_quick_check_instance_of)
STUB_ENTRYPOINT(art_quick_instance_of)
STUB_ENTRYPOINT(art_quick_aput_obj)
STUB_ENTRYPOINT(art_quick_lock_object)
STUB_ENTRYPOINT(art_quick_lock_object_no_inline)
STUB_ENTRYPOINT(art_quick_unlock_object)
STUB_ENTRYPOINT(art_quick_unlock_object_no_inline)
STUB_ENTRYPOINT(art_quick_initialize_static_storage)
STUB_ENTRYPOINT(art_quick_resolve_string)
STUB_ENTRYPOINT(art_quick_resolve_type)
STUB_ENTRYPOINT(art_quick_resolve_type_and_verify_access)
STUB_ENTRYPOINT(art_quick_resolve_method_handle)
STUB_ENTRYPOINT(art_quick_resolve_method_type)
STUB_ENTRYPOINT(art_quick_invoke_custom)
STUB_ENTRYPOINT(art_quick_invoke_polymorphic)
STUB_ENTRYPOINT(art_quick_string_compareto)
STUB_ENTRYPOINT(art_quick_string_builder_append)
STUB_ENTRYPOINT(art_quick_memcpy)
STUB_ENTRYPOINT(art_quick_update_inline_cache)
STUB_ENTRYPOINT(art_quick_instrumentation_entry)
STUB_ENTRYPOINT(art_quick_instrumentation_exit)
STUB_ENTRYPOINT(art_quick_read_barrier_slow)
STUB_ENTRYPOINT(art_quick_read_barrier_for_root_slow)
STUB_ENTRYPOINT(art_invoke_obsolete_method_stub)
STUB_ENTRYPOINT(art_jni_dlsym_lookup_stub)
STUB_ENTRYPOINT(art_jni_dlsym_lookup_critical_stub)

// Quick invoke trampolines
STUB_ENTRYPOINT(art_quick_invoke_direct_trampoline_with_access_check)
STUB_ENTRYPOINT(art_quick_invoke_interface_trampoline_with_access_check)
STUB_ENTRYPOINT(art_quick_invoke_static_trampoline_with_access_check)
STUB_ENTRYPOINT(art_quick_invoke_super_trampoline_with_access_check)
STUB_ENTRYPOINT(art_quick_invoke_virtual_trampoline_with_access_check)

// Throw helpers
STUB_ENTRYPOINT(art_quick_throw_array_bounds)
STUB_ENTRYPOINT(art_quick_throw_div_zero)
STUB_ENTRYPOINT(art_quick_throw_null_pointer_exception)
STUB_ENTRYPOINT(art_quick_throw_stack_overflow)
STUB_ENTRYPOINT(art_quick_throw_string_bounds)

// Math helpers
STUB_ENTRYPOINT(art_quick_ldiv)
STUB_ENTRYPOINT(art_quick_lmod)
STUB_ENTRYPOINT(art_quick_lmul)
STUB_ENTRYPOINT(art_quick_lshl)
STUB_ENTRYPOINT(art_quick_lshr)
STUB_ENTRYPOINT(art_quick_lushr)

// Field access
STUB_ENTRYPOINT(art_quick_get_boolean_static)
STUB_ENTRYPOINT(art_quick_get_byte_static)
STUB_ENTRYPOINT(art_quick_get_char_static)
STUB_ENTRYPOINT(art_quick_get_short_static)
STUB_ENTRYPOINT(art_quick_get32_static)
STUB_ENTRYPOINT(art_quick_get64_static)
STUB_ENTRYPOINT(art_quick_get_obj_static)
STUB_ENTRYPOINT(art_quick_get_boolean_instance)
STUB_ENTRYPOINT(art_quick_get_byte_instance)
STUB_ENTRYPOINT(art_quick_get_char_instance)
STUB_ENTRYPOINT(art_quick_get_short_instance)
STUB_ENTRYPOINT(art_quick_get32_instance)
STUB_ENTRYPOINT(art_quick_get64_instance)
STUB_ENTRYPOINT(art_quick_get_obj_instance)
STUB_ENTRYPOINT(art_quick_set8_static)
STUB_ENTRYPOINT(art_quick_set16_static)
STUB_ENTRYPOINT(art_quick_set32_static)
STUB_ENTRYPOINT(art_quick_set64_static)
STUB_ENTRYPOINT(art_quick_set_obj_static)
STUB_ENTRYPOINT(art_quick_set8_instance)
STUB_ENTRYPOINT(art_quick_set16_instance)
STUB_ENTRYPOINT(art_quick_set32_instance)
STUB_ENTRYPOINT(art_quick_set64_instance)
STUB_ENTRYPOINT(art_quick_set_obj_instance)

// Read barrier mark regs
STUB_ENTRYPOINT(art_quick_read_barrier_mark_reg00)
STUB_ENTRYPOINT(art_quick_read_barrier_mark_reg01)
STUB_ENTRYPOINT(art_quick_read_barrier_mark_reg02)
STUB_ENTRYPOINT(art_quick_read_barrier_mark_reg03)
STUB_ENTRYPOINT(art_quick_read_barrier_mark_reg05)
STUB_ENTRYPOINT(art_quick_read_barrier_mark_reg06)
STUB_ENTRYPOINT(art_quick_read_barrier_mark_reg07)
STUB_ENTRYPOINT(art_quick_read_barrier_mark_reg08)
STUB_ENTRYPOINT(art_quick_read_barrier_mark_reg09)
STUB_ENTRYPOINT(art_quick_read_barrier_mark_reg10)
STUB_ENTRYPOINT(art_quick_read_barrier_mark_reg11)
STUB_ENTRYPOINT(art_quick_read_barrier_mark_reg12)
STUB_ENTRYPOINT(art_quick_read_barrier_mark_reg13)
STUB_ENTRYPOINT(art_quick_read_barrier_mark_reg14)
STUB_ENTRYPOINT(art_quick_read_barrier_mark_reg15)

// Allocation entrypoints - generated per allocator type
// Format: art_quick_alloc_{object_type}_{allocator}[_instrumented]
#define ALLOC_ENTRYPOINTS(suffix) \
  STUB_ENTRYPOINT(art_quick_alloc_array_resolved##suffix) \
  STUB_ENTRYPOINT(art_quick_alloc_array_resolved8##suffix) \
  STUB_ENTRYPOINT(art_quick_alloc_array_resolved16##suffix) \
  STUB_ENTRYPOINT(art_quick_alloc_array_resolved32##suffix) \
  STUB_ENTRYPOINT(art_quick_alloc_array_resolved64##suffix) \
  STUB_ENTRYPOINT(art_quick_alloc_object_resolved##suffix) \
  STUB_ENTRYPOINT(art_quick_alloc_object_initialized##suffix) \
  STUB_ENTRYPOINT(art_quick_alloc_object_with_checks##suffix) \
  STUB_ENTRYPOINT(art_quick_alloc_string_object##suffix) \
  STUB_ENTRYPOINT(art_quick_alloc_string_from_bytes##suffix) \
  STUB_ENTRYPOINT(art_quick_alloc_string_from_chars##suffix) \
  STUB_ENTRYPOINT(art_quick_alloc_string_from_string##suffix)

ALLOC_ENTRYPOINTS(_dlmalloc)
ALLOC_ENTRYPOINTS(_dlmalloc_instrumented)
ALLOC_ENTRYPOINTS(_rosalloc)
ALLOC_ENTRYPOINTS(_rosalloc_instrumented)
ALLOC_ENTRYPOINTS(_bump_pointer)
ALLOC_ENTRYPOINTS(_bump_pointer_instrumented)
ALLOC_ENTRYPOINTS(_tlab)
ALLOC_ENTRYPOINTS(_tlab_instrumented)
ALLOC_ENTRYPOINTS(_region)
ALLOC_ENTRYPOINTS(_region_instrumented)
ALLOC_ENTRYPOINTS(_region_tlab)
ALLOC_ENTRYPOINTS(_region_tlab_instrumented)

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
