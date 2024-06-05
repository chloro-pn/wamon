#pragma once
#include <string_view>

namespace wamon {

inline bool IsReservedId(std::string_view id) { return id.size() > 2 && id.substr(0, 2) == "__"; }

}  // namespace wamon