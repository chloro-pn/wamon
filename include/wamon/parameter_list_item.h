#pragma once

#include <string>

#include "wamon/type.h"

namespace wamon {
struct ParameterListItem {
  std::string name;
  std::unique_ptr<Type> type;
  bool is_ref;
};
}  // namespace wamon