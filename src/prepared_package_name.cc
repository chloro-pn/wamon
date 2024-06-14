#include "wamon/prepared_package_name.h"

namespace wamon {

namespace detail {

template <typename T, size_t N>
constexpr size_t GetArrayLength(T (&)[N]) {
  return N;
}

}  // namespace detail

bool IsPreparedPakageName(const std::string& package_name) {
  static std::string prepared_package_name[] = {
      "wamon",
  };

  for (size_t i = 0; i < detail::GetArrayLength(prepared_package_name); ++i) {
    if (package_name == prepared_package_name[i]) {
      return true;
    }
  }
  return false;
}

}  // namespace wamon