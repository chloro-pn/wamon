#pragma once

#include <string>

namespace wamon {

struct CaptureIdItem {
  std::string id;
  enum class Type { MOVE, REF, NORMAL };
  Type type;
};

}  // namespace wamon