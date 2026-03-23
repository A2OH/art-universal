// link_stubs.cc - Stub implementations for symbols that dex2oat
// references but doesn't actually call during AOT compilation.
// These prevent null-pointer crashes from --unresolved-symbols=ignore-in-object-files.

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <stdarg.h>
#include <dlfcn.h>

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

// __android_log_write - needed by JNIHelp.cpp
int __android_log_write(int priority, const char* tag, const char* text) {
    fprintf(stderr, "%s: %s\n", tag ? tag : "art", text ? text : "");
    fflush(stderr);
    return 0;
}

// __android_log_assert - needed by ALog-priv.h (ALOG_ALWAYS_FATAL_IF)
void __android_log_assert(const char* cond, const char* tag, const char* fmt, ...) {
    va_list ap;
    if (fmt) {
        va_start(ap, fmt);
        fprintf(stderr, "%s: ASSERT FAILED (%s): ", tag ? tag : "art", cond ? cond : "");
        vfprintf(stderr, fmt, ap);
        fprintf(stderr, "\n");
        va_end(ap);
    } else {
        fprintf(stderr, "%s: ASSERT FAILED: %s\n", tag ? tag : "art", cond ? cond : "");
    }
    fflush(stderr);
    abort();
}

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
void* OpenNativeLibrary(void* env, int target_sdk, const char* path, void* class_loader,
                        const char* caller_location, void* library_path,
                        bool* needs_native_bridge, char** error_msg) {
    if (needs_native_bridge) *needs_native_bridge = false;
    if (error_msg) *error_msg = NULL;
    void* handle = dlopen(path, RTLD_NOW);
    if (!handle && error_msg) {
        const char* err = dlerror();
        if (err) *error_msg = strdup(err);
    }
    return handle;
}
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

// === Palette crash stacks ===
int PaletteWriteCrashThreadStacks(const char*, unsigned long) { return 0; }

// === Remaining stub entrypoints NOT provided by real x86_64 assembly ===
// The real quick_entrypoints_x86_64.S, jni_entrypoints_x86_64.S, and
// memcmp16_x86_64.S now provide 244 real symbols. Only stubs for symbols
// not present in x86_64 assembly remain here.

#define STUB_ENTRYPOINT(name) void name() { fprintf(stderr, "FATAL: stub entrypoint " #name " called\n"); abort(); }

// Interpreter stubs (mterp/nterp not in x86_64 quick_entrypoints)
STUB_ENTRYPOINT(ExecuteMterpImpl)
STUB_ENTRYPOINT(ExecuteNterpImpl)

// artMterpAsmInstructionStart/End must be spaced exactly 256*128 = 32768 bytes apart
// for CheckMterpAsmConstants() to pass.
__attribute__((aligned(128))) char artMterpAsmInstructionStart[32768] = {};
char artMterpAsmInstructionEnd[1] = {};

// artNterpAsmInstructionStart/End - same pattern for nterp
__attribute__((aligned(128))) char artNterpAsmInstructionStart[32768] = {};
char artNterpAsmInstructionEnd[1] = {};

// art_quick_invoke_stub_internal is ARM-only; x86_64 uses art_quick_invoke_stub instead
STUB_ENTRYPOINT(art_quick_invoke_stub_internal)

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

// =============================================================================
// === operator<< stubs for ART enums ===
// These are normally auto-generated by art/tools/generate_operator_out.py
// during the AOSP build. We provide minimal stubs that print integer values.
// =============================================================================

#include <atomic>

// --- art:: namespace enums ---
namespace art {

// Forward declarations for enum types (using the exact types from AOSP headers)
enum class InstructionSet;
enum class InvokeType : uint32_t;
enum class LayoutType : uint8_t;
enum class ClassStatus : uint8_t;
enum class ThreadState : uint16_t;
enum class JdwpProvider;
enum class OatClassType : uint8_t;
enum class SuspendReason : uint32_t;
enum class IndirectRefKind;
enum class ReflectionSourceType;
enum VRegKind : int;
enum RootType : int;
enum class JniIdType;

std::ostream& operator<<(std::ostream& os, const InstructionSet& rhs) {
  return os << "InstructionSet(" << static_cast<int>(rhs) << ")";
}
std::ostream& operator<<(std::ostream& os, const InvokeType& rhs) {
  return os << "InvokeType(" << static_cast<int>(rhs) << ")";
}
std::ostream& operator<<(std::ostream& os, const LayoutType& rhs) {
  return os << "LayoutType(" << static_cast<int>(rhs) << ")";
}
std::ostream& operator<<(std::ostream& os, const ClassStatus& rhs) {
  return os << "ClassStatus(" << static_cast<int>(rhs) << ")";
}
std::ostream& operator<<(std::ostream& os, const ThreadState& rhs) {
  return os << "ThreadState(" << static_cast<int>(rhs) << ")";
}
std::ostream& operator<<(std::ostream& os, const JdwpProvider& rhs) {
  return os << "JdwpProvider(" << static_cast<int>(rhs) << ")";
}
std::ostream& operator<<(std::ostream& os, const OatClassType& rhs) {
  return os << "OatClassType(" << static_cast<int>(rhs) << ")";
}
std::ostream& operator<<(std::ostream& os, const SuspendReason& rhs) {
  return os << "SuspendReason(" << static_cast<int>(rhs) << ")";
}
std::ostream& operator<<(std::ostream& os, const IndirectRefKind& rhs) {
  return os << "IndirectRefKind(" << static_cast<int>(rhs) << ")";
}
std::ostream& operator<<(std::ostream& os, const ReflectionSourceType& rhs) {
  return os << "ReflectionSourceType(" << static_cast<int>(rhs) << ")";
}
std::ostream& operator<<(std::ostream& os, const VRegKind& rhs) {
  return os << "VRegKind(" << static_cast<int>(rhs) << ")";
}
std::ostream& operator<<(std::ostream& os, const RootType& rhs) {
  return os << "RootType(" << static_cast<int>(rhs) << ")";
}
std::ostream& operator<<(std::ostream& os, const JniIdType& rhs) {
  return os << "JniIdType(" << static_cast<int>(rhs) << ")";
}

// MethodCompilationStat - in compiler/optimizing
enum class MethodCompilationStat;
std::ostream& operator<<(std::ostream& os, const MethodCompilationStat& rhs) {
  return os << "MethodCompilationStat(" << static_cast<int>(rhs) << ")";
}

// --- ImageHeader nested enums ---
class ImageHeader {
public:
  enum ImageMethod : int;
  enum ImageSections : int;
  enum StorageMode : int;
};
std::ostream& operator<<(std::ostream& os, const ImageHeader::ImageMethod& rhs) {
  return os << "ImageMethod(" << static_cast<int>(rhs) << ")";
}
std::ostream& operator<<(std::ostream& os, const ImageHeader::ImageSections& rhs) {
  return os << "ImageSections(" << static_cast<int>(rhs) << ")";
}
std::ostream& operator<<(std::ostream& os, const ImageHeader::StorageMode& rhs) {
  return os << "StorageMode(" << static_cast<int>(rhs) << ")";
}

// --- EncodedArrayValueIterator nested enum ---
class EncodedArrayValueIterator {
public:
  enum class ValueType : uint8_t;
};
std::ostream& operator<<(std::ostream& os, const EncodedArrayValueIterator::ValueType& rhs) {
  return os << "ValueType(" << static_cast<int>(rhs) << ")";
}

// --- DexLayoutSections nested enum ---
class DexLayoutSections {
public:
  enum class SectionType : uint8_t;
};
std::ostream& operator<<(std::ostream& os, const DexLayoutSections::SectionType& rhs) {
  return os << "SectionType(" << static_cast<int>(rhs) << ")";
}

// --- Location nested enums (compiler/optimizing/locations.h) ---
class Location {
public:
  enum class Kind : uint8_t;
  enum class Policy : uint8_t;
};
std::ostream& operator<<(std::ostream& os, const Location::Kind& rhs) {
  return os << "Location::Kind(" << static_cast<int>(rhs) << ")";
}
std::ostream& operator<<(std::ostream& os, const Location::Policy& rhs) {
  return os << "Location::Policy(" << static_cast<int>(rhs) << ")";
}

// --- LockWord nested enum ---
class LockWord {
public:
  enum LockState : int;
};
std::ostream& operator<<(std::ostream& os, const LockWord::LockState& rhs) {
  return os << "LockState(" << static_cast<int>(rhs) << ")";
}

// --- XzCompress / XzDecompress ---
template <typename T> class ArrayRef;
std::ostream& operator<<(std::ostream&, const ArrayRef<const unsigned char>&);

bool XzCompress(const ArrayRef<const unsigned char>&,
                std::vector<unsigned char>*, int) {
  return false;  // compression not available
}
// By-value overload defined at file end (outside namespace)
bool XzDecompress(const ArrayRef<const unsigned char>&,
                  std::vector<unsigned char>*) {
  return false;  // decompression not available
}
// By-value overload defined at file end (outside namespace)

}  // namespace art

// --- art::gc:: namespace enums ---
namespace art {
namespace gc {

enum class AllocatorType;
enum class CollectorType;
enum class WeakRootState;

std::ostream& operator<<(std::ostream& os, const AllocatorType& rhs) {
  return os << "AllocatorType(" << static_cast<int>(rhs) << ")";
}
std::ostream& operator<<(std::ostream& os, const CollectorType& rhs) {
  return os << "CollectorType(" << static_cast<int>(rhs) << ")";
}
std::ostream& operator<<(std::ostream& os, const WeakRootState& rhs) {
  return os << "WeakRootState(" << static_cast<int>(rhs) << ")";
}

namespace space {

enum class GcRetentionPolicy;
enum class SpaceType;

std::ostream& operator<<(std::ostream& os, const GcRetentionPolicy& rhs) {
  return os << "GcRetentionPolicy(" << static_cast<int>(rhs) << ")";
}
std::ostream& operator<<(std::ostream& os, const SpaceType& rhs) {
  return os << "SpaceType(" << static_cast<int>(rhs) << ")";
}

// RegionSpace nested enums
class RegionSpace {
public:
  enum class RegionType : uint8_t;
  enum class RegionState : uint8_t;
};
std::ostream& operator<<(std::ostream& os, const RegionSpace::RegionType& rhs) {
  return os << "RegionType(" << static_cast<int>(rhs) << ")";
}
std::ostream& operator<<(std::ostream& os, const RegionSpace::RegionState& rhs) {
  return os << "RegionState(" << static_cast<int>(rhs) << ")";
}

}  // namespace space

namespace allocator {

// RosAlloc nested enum
class RosAlloc {
public:
  enum PageMapKind : uint8_t;
};
std::ostream& operator<<(std::ostream& os, const RosAlloc::PageMapKind& rhs) {
  return os << "PageMapKind(" << static_cast<int>(rhs) << ")";
}

}  // namespace allocator

namespace collector {

enum GcType : int;
std::ostream& operator<<(std::ostream& os, const GcType& rhs) {
  return os << "GcType(" << static_cast<int>(rhs) << ")";
}

}  // namespace collector

}  // namespace gc
}  // namespace art

// --- art::instrumentation:: namespace enums ---
namespace art {
namespace instrumentation {

class Instrumentation {
public:
  enum InstrumentationEvent : uint32_t;
  enum class InstrumentationLevel;
};
std::ostream& operator<<(std::ostream& os, const Instrumentation::InstrumentationEvent& rhs) {
  return os << "InstrumentationEvent(0x" << std::hex << static_cast<unsigned>(rhs) << std::dec << ")";
}
std::ostream& operator<<(std::ostream& os, const Instrumentation::InstrumentationLevel& rhs) {
  return os << "InstrumentationLevel(" << static_cast<int>(rhs) << ")";
}

}  // namespace instrumentation
}  // namespace art

// --- art::linker:: namespace enums ---
namespace art {
namespace linker {

class LinkerPatch {
public:
  enum class Type : uint8_t;
};
std::ostream& operator<<(std::ostream& os, const LinkerPatch::Type& rhs) {
  return os << "LinkerPatch::Type(" << static_cast<int>(rhs) << ")";
}

}  // namespace linker
}  // namespace art

// --- art::verifier:: namespace enums ---
namespace art {
namespace verifier {

enum class MethodType : uint32_t;
enum class VerifyError : uint32_t;

std::ostream& operator<<(std::ostream& os, const MethodType& rhs) {
  return os << "MethodType(" << static_cast<int>(rhs) << ")";
}
std::ostream& operator<<(std::ostream& os, const VerifyError& rhs) {
  return os << "VerifyError(0x" << std::hex << static_cast<unsigned>(rhs) << std::dec << ")";
}

}  // namespace verifier
}  // namespace art

// =============================================================================
// === DexCache 128-bit atomic operations ===
// Uses cmpxchg16b on x86_64 (requires -mcx16)
// =============================================================================

namespace art {
namespace mirror {

class DexCache {
public:
  template <typename IntType>
  struct __attribute__((packed)) ConversionPair {
    IntType first;
    IntType second;
  };
  using ConversionPair64 = ConversionPair<uint64_t>;

  static ConversionPair64 AtomicLoadRelaxed16B(std::atomic<ConversionPair64>* target);
  static void AtomicStoreRelease16B(std::atomic<ConversionPair64>* target, ConversionPair64 value);
};

// Use inline cmpxchg16b to avoid depending on __atomic_load_16/__atomic_store_16
// from libatomic (which gets silently resolved to 0 by --unresolved-symbols).

DexCache::ConversionPair64 DexCache::AtomicLoadRelaxed16B(
    std::atomic<ConversionPair64>* target) {
  // cmpxchg16b atomically loads 16 bytes: if [RDI] == RDX:RAX, store RCX:RBX.
  // We use RDX:RAX = 0:0 and RCX:RBX = 0:0 so it never swaps, just loads.
  uint64_t lo, hi;
  __asm__ __volatile__(
      "xorq %%rax, %%rax\n\t"
      "xorq %%rdx, %%rdx\n\t"
      "xorq %%rbx, %%rbx\n\t"
      "xorq %%rcx, %%rcx\n\t"
      "lock cmpxchg16b (%[ptr])\n\t"
      : "=a"(lo), "=d"(hi)
      : [ptr] "r"(target)
      : "rbx", "rcx", "cc", "memory");
  ConversionPair64 result;
  result.first = lo;
  result.second = hi;
  return result;
}

void DexCache::AtomicStoreRelease16B(
    std::atomic<ConversionPair64>* target, ConversionPair64 value) {
  // Atomic store via cmpxchg16b loop: read old, try to swap in new.
  uint64_t new_lo = value.first;
  uint64_t new_hi = value.second;
  uint64_t old_lo, old_hi;
  // First, read current value
  __asm__ __volatile__(
      "xorq %%rax, %%rax\n\t"
      "xorq %%rdx, %%rdx\n\t"
      "xorq %%rbx, %%rbx\n\t"
      "xorq %%rcx, %%rcx\n\t"
      "lock cmpxchg16b (%[ptr])\n\t"
      : "=a"(old_lo), "=d"(old_hi)
      : [ptr] "r"(target)
      : "rbx", "rcx", "cc", "memory");
  // Loop until we successfully store
  bool success;
  do {
    __asm__ __volatile__(
        "lock cmpxchg16b (%[ptr])\n\t"
        "setz %[success]\n\t"
        : "=a"(old_lo), "=d"(old_hi), [success] "=r"(success)
        : "a"(old_lo), "d"(old_hi), "b"(new_lo), "c"(new_hi), [ptr] "r"(target)
        : "cc", "memory");
  } while (!success);
}

}  // namespace mirror
}  // namespace art

// =============================================================================
// === JNI native method stubs (from jni_entrypoints.cc, failed to compile) ===
// =============================================================================

extern "C" {

// when a native method needs to be resolved. In dex2oat --compiler-filter=verify
// mode, these should never be reached. If called, return null (will trigger exception).
  return nullptr;
}
  return nullptr;
}
  return 0;
}

}  // extern "C"

// XzCompress/XzDecompress by-value overloads (for JIT compiler)
// These use mangled names directly to avoid needing ArrayRef definition
extern "C" {
bool _ZN3art10XzCompressENS_8ArrayRefIKhEEPNSt3__16vectorIhNS3_9allocatorIhEEEEi(
    void* arrayref, void* output, int level) {
  return false;
}
bool _ZN3art12XzDecompressENS_8ArrayRefIKhEEPNSt3__16vectorIhNS3_9allocatorIhEEEE(
    void* arrayref, void* output) {
  return false;
}
}
