#include "wamon/operator.h"

namespace wamon {

static void register_buildin_operators(std::unordered_map<Token, int>& ops) {
  ops[Token::COMPARE] = 0;
  ops[Token::ASSIGN] = 0;
  ops[Token::PLUS] = 1;
  ops[Token::MINUS] = 1;
  ops[Token::MULTIPLY] = 2;
  ops[Token::DIVIDE] = 2;
  ops[Token::AND] = 5;
  ops[Token::OR] = 4;
  ops[Token::PIPE] = 6;
  ops[Token::MEMBER_ACCESS] = 7;
  ops[Token::SUBSCRIPT] = 7;
}

static void register_buildin_u_operators(std::unordered_map<Token, int>& ops) {
  // -
  ops[Token::MINUS] = 0;
  // *
  ops[Token::MULTIPLY] = 0;
  // &
  ops[Token::ADDRESS_OF] = 0;
  // !
  ops[Token::NOT] = 0;
}

Operator::Operator() { 
  register_buildin_operators(operators_); 
  register_buildin_u_operators(uoperators_); 
}
}  // namespace wamon