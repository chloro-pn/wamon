#pragma once

#include <unordered_map>

#include "wamon/token.h"

namespace wamon {

class Operator {
 public:
  Operator();

  static Operator& Instance() {
    static Operator op;
    return op;
  }

  bool FindBinary(Token token) const { return operators_.count(token) > 0; }

  int GetLevel(Token token) const { return operators_.at(token); }

 private:
  // operator -> 优先级
  std::unordered_map<Token, int> operators_;
};

}  // namespace wamon