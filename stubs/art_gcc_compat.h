// art_gcc_compat.h - Pre-included header for GCC compatibility with ART
// This must be -include'd before any ART/AOSP headers

#ifndef ART_GCC_COMPAT_H
#define ART_GCC_COMPAT_H

// Force standard library headers that ART code assumes are available
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <functional>
#include <signal.h>

// Prevent AOSP strlcpy.h from conflicting with glibc's strlcpy (glibc 2.38+)
// The system already provides strlcpy, so skip AOSP's static inline version.
#define ART_LIBARTBASE_BASE_STRLCPY_H_

// ART image base address ASLR deltas (used by image_space.cc)
#ifndef ART_BASE_ADDRESS_MIN_DELTA
#define ART_BASE_ADDRESS_MIN_DELTA (-0x1000000)
#endif
#ifndef ART_BASE_ADDRESS_MAX_DELTA
#define ART_BASE_ADDRESS_MAX_DELTA 0x1000000
#endif

// ART frame size limit (used by instruction_set.cc)
#ifndef ART_FRAME_SIZE_LIMIT
#define ART_FRAME_SIZE_LIMIT 7400
#endif

// IMT (Interface Method Table) size (used by runtime headers)
#ifndef IMT_SIZE
#define IMT_SIZE 43
#endif

// For GCC compatibility with offsetof
#include <cstdint>

// NOTE: object_array-inl.h is now included (needed by runtime's heap.cc).
// AOSP clang handles the const-correctness in cbegin/cend fine.

// ART compact DEX level (needed by dex2oat)
#ifndef ART_DEFAULT_COMPACT_DEX_LEVEL
#define ART_DEFAULT_COMPACT_DEX_LEVEL fast
#endif

// Enable all code generators
#ifndef ART_ENABLE_CODEGEN_arm
#define ART_ENABLE_CODEGEN_arm
#endif
#ifndef ART_ENABLE_CODEGEN_arm64
#define ART_ENABLE_CODEGEN_arm64
#endif
#ifndef ART_ENABLE_CODEGEN_x86
#define ART_ENABLE_CODEGEN_x86
#endif
#ifndef ART_ENABLE_CODEGEN_x86_64
#define ART_ENABLE_CODEGEN_x86_64
#endif

// Ensure we're a host build - undefine any target/android macros
#ifdef ART_TARGET
#undef ART_TARGET
#endif
#ifdef ART_TARGET_ANDROID
#undef ART_TARGET_ANDROID
#endif
#ifdef __ANDROID__
#undef __ANDROID__
#endif
#ifdef ANDROID
#undef ANDROID
#endif

#endif  // ART_GCC_COMPAT_H
