#pragma once

#include <exception>
#include <string>
#include <utility>
#include <vector>

#include "fmt/format.h"

namespace wamon {

class WamonExecption : public std::exception {
 public:
  template <typename... Args>
  explicit WamonExecption(fmt::format_string<Args...> formator, Args&&... what) : what_() {
    what_ = fmt::format(formator, std::forward<Args>(what)...);
  }

  explicit WamonExecption(const char* formator) : what_(formator) {}

  const char* what() const noexcept override { return what_.c_str(); }

 private:
  std::string what_;
};
}  // namespace bptree