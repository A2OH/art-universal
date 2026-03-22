// Stub for android-base/properties.h - host build
#ifndef STUB_ANDROID_BASE_PROPERTIES_H
#define STUB_ANDROID_BASE_PROPERTIES_H

#include <string>

namespace android {
namespace base {

static inline std::string GetProperty(const std::string& key, const std::string& default_value) {
  return default_value;
}

static inline bool GetBoolProperty(const std::string& key, bool default_value) {
  return default_value;
}

static inline int GetIntProperty(const std::string& key, int default_value,
                                  int min = INT_MIN, int max = INT_MAX) {
  return default_value;
}

static inline bool SetProperty(const std::string& key, const std::string& value) {
  return false;
}

static inline bool WaitForProperty(const std::string& key, const std::string& expected_value,
                                    std::chrono::milliseconds relative_timeout = std::chrono::milliseconds::max()) {
  return false;
}

}  // namespace base
}  // namespace android

#endif
