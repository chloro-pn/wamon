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

  bool FindUnary(Token token) const { return uoperators_.count(token) > 0; }

  int GetLevel(Token token) const { return operators_.at(token); }

  // 查询运算符是否允许被重载
  bool CanBeOverride(Token token) const {
    // todo implement
    return true;
  }

 private:
  // 双元运算符 -> 优先级
  std::unordered_map<Token, int> operators_;
  // 单元运算符具有最高优先级，并且是左结合的，并且单元运算符的优先级一致
  std::unordered_map<Token, int> uoperators_;
};

}  // namespace wamon