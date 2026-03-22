// Shadow thread_annotations.h - GCC-compatible version
// All annotations become no-ops since GCC doesn't support clang thread safety attributes

#ifndef STUB_THREAD_ANNOTATIONS_H_
#define STUB_THREAD_ANNOTATIONS_H_

#include <mutex>

#define THREAD_ANNOTATION_ATTRIBUTE__(x)

#define CAPABILITY(x)
#define SCOPED_CAPABILITY
#define SCOPED_LOCKABLE
#define SHARED_CAPABILITY(...)
#define GUARDED_BY(x)
#define PT_GUARDED_BY(x)
#define EXCLUSIVE_LOCKS_REQUIRED(...)
#define SHARED_LOCKS_REQUIRED(...)
#define ACQUIRED_BEFORE(...)
#define ACQUIRED_AFTER(...)
#define REQUIRES(...)
#define REQUIRES_SHARED(...)
#define ACQUIRE(...)
#define ACQUIRE_SHARED(...)
#define RELEASE(...)
#define RELEASE_SHARED(...)
#define TRY_ACQUIRE(...)
#define TRY_ACQUIRE_SHARED(...)
#define EXCLUDES(...)
#define ASSERT_CAPABILITY(x)
#define ASSERT_SHARED_CAPABILITY(x)
#define RETURN_CAPABILITY(x)
#define EXCLUSIVE_LOCK_FUNCTION(...)
#define EXCLUSIVE_TRYLOCK_FUNCTION(...)
#define SHARED_LOCK_FUNCTION(...)
#define SHARED_TRYLOCK_FUNCTION(...)
#define UNLOCK_FUNCTION(...)
#define LOCK_RETURNED(x)
#define NO_THREAD_SAFETY_ANALYSIS

namespace android {
namespace base {

class ScopedLockAssertion {
 public:
  ScopedLockAssertion(std::mutex& mutex) {}
  ~ScopedLockAssertion() {}
};

}  // namespace base
}  // namespace android

#endif  // STUB_THREAD_ANNOTATIONS_H_
