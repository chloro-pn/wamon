#include "wamon/parser.h"

#include <stdexcept>

#include "fmt/format.h"

namespace wamon {
/*
 * @brief 判断tokens[begin]处的token值 == token
 */
bool AssertToken(const std::vector<WamonToken> &tokens, size_t &begin, Token token) {
  if (tokens.size() <= begin || tokens[begin].token != token) {
    return false;
  }
  begin += 1;
  return true;
}

void AssertTokenOrThrow(const std::vector<WamonToken> &tokens, size_t &begin, Token token) {
  if (tokens.size() <= begin || tokens[begin].token != token) {
    throw std::runtime_error(
        fmt::format("parse token error, {} != {}", GetTokenStr(token), GetTokenStr(tokens[begin].token)));
  }
  begin += 1;
}

std::string ParseIdentifier(const std::vector<WamonToken> &tokens, size_t &begin) {
  if (tokens.size() <= begin || tokens[begin].token != Token::ID || tokens[begin].type != WamonToken::ValueType::STR) {
    throw std::runtime_error("parse identifier error");
  }
  std::string ret = tokens[begin].Get<std::string>();
  begin += 1;
  return ret;
}

std::string ParseBasicType(const std::vector<WamonToken> &tokens, size_t &begin) {
  if (tokens.size() <= begin) {
    throw std::runtime_error("parse basictype error");
  }
  std::string ret;
  if (tokens[begin].token == Token::STRING) {
    ret = "string";
  } else if (tokens[begin].token == Token::INT) {
    ret = "int";
  } else if (tokens[begin].token == Token::BYTE) {
    ret = "byte";
  } else if (tokens[begin].token == Token::BOOL) {
    ret = "bool";
  } else if (tokens[begin].token == Token::DOUBLE) {
    ret = "double";
  } else if (tokens[begin].token == Token::VOID) {
    ret = "void";
  }
  if (ret != "") {
    begin += 1;
  }
  return ret;
}

// 从tokens[begin]开始解析一个类型信息，更新begin，并返回解析到的类型
std::unique_ptr<Type> ParseType(const std::vector<WamonToken> &tokens, size_t &begin) {
  std::unique_ptr<Type> ret;
  std::string type_name = ParseBasicType(tokens, begin);
  if (type_name == "") {
    type_name = ParseIdentifier(tokens, begin);
  }
  ret = std::make_unique<BasicType>(type_name);
  // todo 支持复合类型
  return ret;
}

std::unique_ptr<Expression> ParseExpression(const std::vector<WamonToken>& tokens, size_t begin, size_t end) {
  return std::make_unique<Expression>();
}

std::unique_ptr<Statement> ParseStatement(const std::vector<WamonToken>& tokens, size_t begin, size_t end) {
  return std::make_unique<Statement>();
}

std::vector<std::unique_ptr<Statement>> ParseStmtBlock(const std::vector<WamonToken>& tokens, size_t begin, size_t end) {
  return std::vector<std::unique_ptr<Statement>>();
}

//   (  type id, type id, ... )
// begin                     end
std::vector<std::pair<std::string, std::unique_ptr<Type>>> ParseParameterList(const std::vector<WamonToken>& tokens, size_t begin, size_t end) {
  std::vector<std::pair<std::string, std::unique_ptr<Type>>> ret;
  AssertTokenOrThrow(tokens, begin, Token::LEFT_PARENTHESIS);
  if (AssertToken(tokens, begin, Token::RIGHT_PARENTHESIS)) {
    return ret;
  }
  while(true) {
    auto type = ParseType(tokens, begin);
    std::string id = ParseIdentifier(tokens, begin);
    ret.push_back(std::pair<std::string, std::unique_ptr<Type>>(id, std::move(type)));
    bool succ = AssertToken(tokens, begin, Token::COMMA);
    if (succ == false) {
      AssertTokenOrThrow(tokens, begin, Token::RIGHT_PARENTHESIS);
      break;
    }
  }
  if (begin != end + 1) {
    throw std::runtime_error(fmt::format("parse parameter list error, {} != {}", begin, end+1));
  }
  return ret;
}

//   (  expr1, expr2, expr3, ...   )
// begin                          end
std::vector<std::unique_ptr<Expression>> ParseExprList(const std::vector<WamonToken>& tokens, size_t begin, size_t end) {
  std::vector<std::unique_ptr<Expression>> ret;
  size_t current = begin;
  while(current != end) {
    size_t next = FindNextToken<Token::COMMA>(tokens, current, end);
    auto expr = ParseExpression(tokens, current + 1, next);
    ret.push_back(std::move(expr));
    current = next;
  }
  return ret;
}

void TryToParseFunctionDeclaration(const std::vector<WamonToken> &tokens, size_t& begin) {
  bool succ = AssertToken(tokens, begin, Token::FUNC);
  if (succ == false) {
    return;
  }
  std::string func_name = ParseIdentifier(tokens, begin);
  size_t end = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
  auto param_list = ParseParameterList(tokens, begin, end);
  begin = end + 1;
  AssertTokenOrThrow(tokens, begin, Token::ARROW);
  auto return_type = ParseType(tokens, begin);
  end = FindMatchedToken<Token::LEFT_BRACE, Token::RIGHT_BRACE>(tokens, begin);
  auto stmt_block = ParseStmtBlock(tokens, begin, end);
  begin = end + 1;
  return;
}

void TryToParseStructDeclaration(const std::vector<WamonToken> &tokens, size_t& begin) {
  bool succ = AssertToken(tokens, begin, Token::STRUCT);
  if (succ == false) {
    return;
  }
  std::string struct_name = ParseIdentifier(tokens, begin);
  AssertTokenOrThrow(tokens, begin, Token::LEFT_BRACE);
  while(AssertToken(tokens, begin, Token::RIGHT_BRACE) == false) {
    auto type = ParseType(tokens, begin);
    auto field_name = ParseIdentifier(tokens, begin);
    AssertTokenOrThrow(tokens, begin, Token::SEMICOLON);
  }
  return;
}

// let var_name : type = (expr_list) | expr;
void TryToParseVariableDeclaration(const std::vector<WamonToken> &tokens, size_t& begin) {
  bool succ = AssertToken(tokens, begin, Token::LET);
  if (succ == false) {
    return;
  }
  std::string var_name = ParseIdentifier(tokens, begin);
  AssertTokenOrThrow(tokens, begin, Token::SEMICOLON);
  auto type = ParseType(tokens, begin);
  AssertTokenOrThrow(tokens, begin, Token::ASSIGN);
  // parse expr list.
  size_t end = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
  ParseExprList(tokens, begin, end);
  begin = end + 1;
  AssertTokenOrThrow(tokens, begin, Token::SEMICOLON);
  return;
}

void Parse(const std::vector<WamonToken> &tokens) {
  if (tokens.empty() == true || tokens.back().token != Token::TEOF) {
    throw std::runtime_error("invalid tokens");
  }
  size_t current_index = 0;
  while (current_index < tokens.size()) {
    WamonToken token = tokens[current_index];
    if (token.token == Token::TEOF) {
      break;
    }
    size_t old_index = current_index;
    TryToParseFunctionDeclaration(tokens, current_index);
    TryToParseStructDeclaration(tokens, current_index);
    TryToParseVariableDeclaration(tokens, current_index);
    if (old_index == current_index) {
      throw std::runtime_error("parse error");
    }
  }
}

}  // namespace wamon