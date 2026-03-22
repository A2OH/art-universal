// Shadow mirror/dex_cache-inl.h
// Ensures our dex_cache.h shadow (which disables x86_64 asm) is included first,
// then delegates to the real dex_cache-inl.h.
#ifndef STUB_MIRROR_DEX_CACHE_INL_WRAPPER_H_
#define STUB_MIRROR_DEX_CACHE_INL_WRAPPER_H_

// Include our shadow dex_cache.h first to set the include guard
// with __x86_64__ undefined
#include "mirror/dex_cache.h"

#pragma GCC system_header
#include_next "mirror/dex_cache-inl.h"

#endif  // STUB_MIRROR_DEX_CACHE_INL_WRAPPER_H_
