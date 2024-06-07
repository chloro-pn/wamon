#pragma once

#include <stdexcept>

#include "wamon/exception.h"

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
  ELIF,
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
  COMMA,
  ARROW,
  CALL,

  // 运算符
  PLUS,
  MINUS,
  INCREMENT,
  MULTIPLY,
  DIVIDE,
  AND,
  OR,
  NOT,
  ASSIGN,
  COMPARE,
  GT,   // GREATER_THAN
  LT,   // LESS_THAN,
  GTE,  // GREATER_THAN_OR_EQ
  LTE,  // LESS_THAN_OR_EQ
  ADDRESS_OF,
  PIPE,
  MEMBER_ACCESS,  // a.b == a MEMBER_ACCESS b
  AS,

  // 特殊的TOKEN，不会在词法分析阶段出现，而是将一些语法等价为二元运算符
  SUBSCRIPT,  // a[b] == a SUBSCRIPT b

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
  // 运算符重载
  OPERATOR,
  // 结构体定义
  STRUCT,
  TRAIT,
  DESTRUCTOR,
  // 变量定义
  LET,
  // 变量引用
  REF,
  // 匿名变量定义
  NEW,
  // 堆上变量定义和销毁
  ALLOC,
  DEALLOC,
  // 方法定义
  METHOD,
  SELF,
  // lambda
  LAMBDA,

  // 复合类型声明
  PTR,
  LIST,
  F,

  // 移动语义
  MOVE,

  // 包管理
  PACKAGE,
  IMPORT,
  SCOPE,

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
    case Token::ELIF:
      return "elif";
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
    case Token::COMMA:
      return ",";
    case Token::ARROW:
      return "->";
    case Token::CALL:
      return "call";
    case Token::PLUS:
      return "+";
    case Token::MINUS:
      return "-";
    case Token::INCREMENT:
      return "++";
    case Token::MULTIPLY:
      return "*";
    case Token::DIVIDE:
      return "/";
    case Token::AND:
      return "&&";
    case Token::OR:
      return "||";
    case Token::NOT:
      return "!";
    case Token::ASSIGN:
      return "=";
    case Token::COMPARE:
      return "==";
    case Token::GT:
      return ">";
    case Token::GTE:
      return ">=";
    case Token::LT:
      return "<";
    case Token::LTE:
      return "<=";
    case Token::ADDRESS_OF:
      return "&";
    case Token::PIPE:
      return "|";
    case Token::SUBSCRIPT:
      return "[]";
    case Token::MEMBER_ACCESS:
      return ".";
    case Token::AS:
      return "as";
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
    case Token::OPERATOR:
      return "operator";
    case Token::STRUCT:
      return "struct";
    case Token::TRAIT:
      return "trait";
    case Token::DESTRUCTOR:
      return "destructor";
    case Token::LET:
      return "let";
    case Token::REF:
      return "ref";
    case Token::NEW:
      return "new";
    case Token::ALLOC:
      return "alloc";
    case Token::DEALLOC:
      return "dealloc";
    case Token::METHOD:
      return "method";
    case Token::SELF:
      return "self";
    case Token::LAMBDA:
      return "lambda";
    case Token::PTR:
      return "ptr";
    case Token::LIST:
      return "list";
    case Token::F:
      return "f";
    case Token::MOVE:
      return "move";
    case Token::PACKAGE:
      return "package";
    case Token::IMPORT:
      return "import";
    case Token::SCOPE:
      return "::";
    case Token::TEOF:
      return "TEOF";
    default:
      throw WamonException("invalid token");
  }
}

}  // namespace wamon
