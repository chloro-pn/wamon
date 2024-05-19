#pragma once

#include <functional>
#include <unordered_map>

#include "wamon/operator_def.h"
#include "wamon/token.h"
#include "wamon/variable.h"

namespace wamon {

class Interpreter;

class Operator {
 public:
  static Operator& Instance() {
    static Operator op;
    return op;
  }

  bool FindBinary(Token token) const { return operators_.count(token) > 0; }

  bool FindUnary(Token token) const { return uoperators_.count(token) > 0; }

  int GetLevel(Token token) const { return operators_.at(token); }

  // 查询运算符是否允许被重载
  bool CanBeOverride(Token token) const {
    if (token == Token::COMPARE || token == Token::PLUS || token == Token::MINUS || token == Token::MULTIPLY ||
        token == Token::DIVIDE || token == Token::AND || token == Token::OR || token == Token::PIPE) {
      return true;
    }
    return false;
  }

  std::shared_ptr<Variable> Calculate(Interpreter& interpreter, Token op, std::shared_ptr<Variable> v1,
                                      std::shared_ptr<Variable> v2) const;

  std::shared_ptr<Variable> Calculate(Interpreter& interpreter, Token op, std::shared_ptr<Variable> v) const;

  using BinaryOperatorType =
      std::function<std::shared_ptr<Variable>(Interpreter&, std::shared_ptr<Variable>, std::shared_ptr<Variable>)>;

  using UnaryOperatorType = std::function<std::shared_ptr<Variable>(std::shared_ptr<Variable>)>;

 private:
  Operator();
  // 双元运算符 -> 优先级
  std::unordered_map<Token, int> operators_;
  // 单元运算符具有最高优先级，并且是左结合的，并且单元运算符的优先级一致
  std::unordered_map<Token, int> uoperators_;

  std::unordered_map<std::string, BinaryOperatorType> operator_handles_;

  std::unordered_map<std::string, UnaryOperatorType> uoperator_handles_;
};

}  // namespace wamon
