// Shadow mirror/dex_cache.h - route around GCC cmpxchg16b asm constraint issue
// The real dex_cache.h has inline x86_64 asm that GCC can't allocate registers for.
// Fix: temporarily undefine __x86_64__ so it falls through to the #else branch
// which just declares the functions without inline asm.
#ifndef STUB_MIRROR_DEX_CACHE_WRAPPER_H_
#define STUB_MIRROR_DEX_CACHE_WRAPPER_H_

#pragma push_macro("__x86_64__")
#undef __x86_64__

#pragma GCC system_header
#include_next "mirror/dex_cache.h"

#pragma pop_macro("__x86_64__")

#endif  // STUB_MIRROR_DEX_CACHE_WRAPPER_H_
