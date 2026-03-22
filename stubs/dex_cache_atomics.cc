// Standalone definitions for DexCache 16-byte atomic operations.
// On x86_64, the ALWAYS_INLINE versions use cmpxchg16b but aren't
// emitted as standalone symbols. This file provides them.

#include "mirror/dex_cache.h"
#include <cstring>

namespace art {
namespace mirror {

DexCache::ConversionPair64 DexCache::AtomicLoadRelaxed16B(
    std::atomic<ConversionPair64>* target) {
  __int128 val;
  __atomic_load(reinterpret_cast<__int128*>(target), &val, __ATOMIC_RELAXED);
  ConversionPair64 result;
  memcpy(&result, &val, sizeof(result));
  return result;
}

void DexCache::AtomicStoreRelease16B(
    std::atomic<ConversionPair64>* target, ConversionPair64 value) {
  __int128 val;
  memcpy(&val, &value, sizeof(val));
  __atomic_store(reinterpret_cast<__int128*>(target), &val, __ATOMIC_RELEASE);
}

}  // namespace mirror
}  // namespace art
