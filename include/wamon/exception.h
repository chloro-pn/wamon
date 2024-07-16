#pragma once

#include <exception>
#include <string>
#include <utility>

#include "fmt/format.h"

namespace wamon {

class WamonException : public std::exception {
 public:
  template <typename... Args>
  explicit WamonException(fmt::format_string<Args...> formator, Args&&... what) : what_() {
    what_ = fmt::format(formator, std::forward<Args>(what)...);
  }

  explicit WamonException(const char* formator) : what_(formator) {}

  const char* what() const noexcept override { return what_.c_str(); }

 private:
  std::string what_;
};

class WamonDeterministicReturn : public WamonException {
 public:
  explicit WamonDeterministicReturn(const std::string& func_name)
      : WamonException("deterministic return check error , {}", func_name) {}
};

class WamonTypeCheck : public WamonException {
 public:
  explicit WamonTypeCheck(const std::string& context_info, const std::string& type_info, const std::string& reason)
      : WamonException("type check error, context : {}, type_info : {}, reason : {}", context_info, type_info, reason) {

  }
};

class WamonCompareTypeDismatch : public WamonException {
 public:
  explicit WamonCompareTypeDismatch(const std::string& t1, const std::string& t2)
      : WamonException("wamon compare type dismatch : {} != {}", t1, t2) {}
};

inline void Unreachable(const char* file, int line) {
  throw WamonException("wamon unreachable error : {} {}", file, line);
}

#define WAMON_UNREACHABLE Unreachable(__FILE__, __LINE__);
}  // namespace wamon
