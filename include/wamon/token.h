#pragma once

#include <stdexcept>

namespace wamon {

// wamon语言的token列表
enum class Token {
  // 空白字符
  SPACE,
  TAB,
  ENTER,
  LINEFEED,

  // 基础类型标识符
  INT,
  DOUBLE,
  BYTE,
  STRING,
  BOOL,
  VOID,

  // 流程控制
  IF,
  ELSE,
  WHILE,
  FOR,
  BREAK,
  CONTINUE,
  RETURN,

  // 括号
  LEFT_PARENTHESIS,
  RIGHT_PARENTHESIS,
  LEFT_BRACKETS,
  RIGHT_BRACKETS,
  LEFT_BRACE,
  RIGHT_BRACE,

  // 格式控制
  SEMICOLON,
  COLON,
  DECIMAL_POINT,
  COMMA,

  // 运算符
  PLUS,
  MINUS,
  MULTIPLY,
  DIVIDE,
  AND,
  OR,
  NOT,
  ASSIGN,
  COMPARE,
  GT,  // GREATER_THAN
  LT,  // LESS_THAN,
  ADDRESS_OF,

  // 自定义标识符和字面量
  ID,
  STRING_ITERAL,
  INT_ITERAL,
  DOUBLE_ITERAL,
  TRUE,
  FALSE,
  BYTE_ITERAL,

  // 函数定义
  FUNC,
  // 结构体定义
  STRUCT,
  // 变量定义
  LET,

  // 标志文件结尾，由词法分析器添加
  TEOF,
  INVALID,
};

inline const char *GetTokenStr(Token token) {
  switch (token) {
    case Token::SPACE:
      return "space";
    case Token::TAB:
      return "tab";
    case Token::ENTER:
      return "enter";
    case Token::LINEFEED:
      return "linefeed";
    case Token::INT:
      return "int";
    case Token::DOUBLE:
      return "double";
    case Token::BYTE:
      return "byte";
    case Token::STRING:
      return "string";
    case Token::BOOL:
      return "bool";
    case Token::VOID:
      return "void";
    case Token::IF:
      return "if";
    case Token::ELSE:
      return "else";
    case Token::WHILE:
      return "while";
    case Token::FOR:
      return "for";
    case Token::BREAK:
      return "break";
    case Token::CONTINUE:
      return "continue";
    case Token::RETURN:
      return "return";
    case Token::LEFT_PARENTHESIS:
      return "(";
    case Token::RIGHT_PARENTHESIS:
      return ")";
    case Token::LEFT_BRACKETS:
      return "[";
    case Token::RIGHT_BRACKETS:
      return "]";
    case Token::LEFT_BRACE:
      return "{";
    case Token::RIGHT_BRACE:
      return "}";
    case Token::SEMICOLON:
      return ";";
    case Token::COLON:
      return ":";
    case Token::DECIMAL_POINT:
      return ".";
    case Token::COMMA:
      return ",";
    case Token::PLUS:
      return "+";
    case Token::MINUS:
      return "-";
    case Token::MULTIPLY:
      return "*";
    case Token::DIVIDE:
      return "/";
    case Token::AND:
      return "&";
    case Token::OR:
      return "|";
    case Token::NOT:
      return "!";
    case Token::ASSIGN:
      return "=";
    case Token::COMPARE:
      return "==";
    case Token::GT:
      return ">";
    case Token::LT:
      return "<";
    case Token::ADDRESS_OF:
      return "addr";
    case Token::ID:
      return "id";
    case Token::STRING_ITERAL:
      return "string_iteral";
    case Token::INT_ITERAL:
      return "int_iteral";
    case Token::DOUBLE_ITERAL:
      return "double_iteral";
    case Token::TRUE:
      return "true";
    case Token::FALSE:
      return "false";
    case Token::BYTE_ITERAL:
      return "byte_iteral";
    case Token::FUNC:
      return "func";
    case Token::STRUCT:
      return "struct";
    case Token::LET:
      return "let";
    case Token::TEOF:
      return "TEOF";
    default:
      throw std::runtime_error("invalid token");
  }
}

}  // namespace wamon