#include "wamon/operator.h"

namespace wamon {

static void register_buildin_operators(std::unordered_map<Token, int>& ops) {
  ops[Token::PLUS] = 1;
  ops[Token::MINUS] = 1;
  ops[Token::MULTIPLY] = 2;
  ops[Token::DIVIDE] = 2;
  ops[Token::AND] = 5;
  ops[Token::OR] = 4;
  ops[Token::NOT] = 3;
  ops[Token::ASSIGN] = 6;
}

Operator::Operator() { register_buildin_operators(operators_); }
}  // namespace wamon