// Shadow base/macros.h - wraps real one and overrides OFFSETOF_MEMBER for GCC
#ifndef STUB_BASE_MACROS_WRAPPER_H_
#define STUB_BASE_MACROS_WRAPPER_H_

// Use include_next to find the real base/macros.h in subsequent -I paths
#pragma GCC system_header
#include_next "base/macros.h"

// Override OFFSETOF_MEMBER with GCC-compatible version.
// Uses __builtin_offsetof which works for plain members in constexpr context.
// For array subscripts with variable indices (runtime.h:461), GCC will error
// but we handle that by adding -Wno-error and relying on the template
// not being instantiated at compile time.
#undef OFFSETOF_MEMBER
#define OFFSETOF_MEMBER(t, f) __builtin_offsetof(t, f)

// GCC 13 cannot allocate registers for cmpxchg16b inline asm when the function
// is marked always_inline (dex_cache.h:539). Since this is a host compiler build,
// removing always_inline is safe - GCC -O2 will still inline where profitable.
#undef ALWAYS_INLINE
#define ALWAYS_INLINE

#endif  // STUB_BASE_MACROS_WRAPPER_H_
