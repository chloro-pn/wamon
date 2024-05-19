#pragma once

#include <numeric>
#include <string>
#include <vector>

#include "wamon/ast.h"
#include "wamon/exception.h"
#include "wamon/function_def.h"
#include "wamon/package_unit.h"
#include "wamon/scanner.h"
#include "wamon/struct_def.h"

namespace wamon {

void AssertTokenOrThrow(const std::vector<WamonToken> &tokens, size_t &begin, Token token, const char *, int);

/*
 * @brief 需要匹配的token，tokens[begin]必须合法，并且tokens[begin] == left。
 * 如果找不到匹配的token，抛出异常。
 */
template <Token left, Token right>
size_t FindMatchedToken(const std::vector<WamonToken> &tokens, size_t begin) {
  AssertTokenOrThrow(tokens, begin, left, __FILE__, __LINE__);
  size_t counter = 1;
  size_t index = begin;
  while (index < tokens.size()) {
    if (tokens[index].token == left) {
      ++counter;
    } else if (tokens[index].token == right) {
      --counter;
      if (counter == 0) {
        return index;
      }
    }
    ++index;
  }
  throw WamonExecption("find matched token : {} - {} error", GetTokenStr(left), GetTokenStr(right));
}

// 在区间(begin, end]中找到第一个值为token的token，注意需要跳过所有不匹配的大中小括号，如果找不到的话，返回end。
template <Token token, bool skip_first_token = true, bool throw_if_not_found = false>
size_t FindNextToken(const std::vector<WamonToken> &tokens, size_t begin,
                     size_t end = std::numeric_limits<size_t>::max()) {
  if (end != std::numeric_limits<size_t>::max()) {
    if (tokens.size() <= end || tokens[end].token == token) {
      throw WamonExecption("find next token error");
    }
  }
  if constexpr (skip_first_token) {
    begin += 1;
  }
  end = std::min(end, tokens.size() - 1);
  size_t counter1 = 0;  // ()
  size_t counter2 = 0;  // []
  size_t counter3 = 0;  // {}
  for (size_t i = begin; i <= end; ++i) {
    if (tokens[i].token == token && counter1 == 0 && counter2 == 0 && counter3 == 0) {
      return i;
    }
    if (tokens[i].token == Token::LEFT_PARENTHESIS) {
      ++counter1;
    } else if (tokens[i].token == Token::RIGHT_PARENTHESIS) {
      --counter1;
    } else if (tokens[i].token == Token::LEFT_BRACKETS) {
      ++counter2;
    } else if (tokens[i].token == Token::RIGHT_BRACKETS) {
      --counter2;
    } else if (tokens[i].token == Token::LEFT_BRACE) {
      ++counter3;
    } else if (tokens[i].token == Token::RIGHT_BRACE) {
      --counter3;
    }
  }
  if constexpr (throw_if_not_found) {
    throw WamonExecption("FindNextToken error, not found {}", GetTokenStr(token));
  }
  return end;
}

// 在区间[begin, end]中查找符号token，如果找到了则finded设置为true，否则设置为false。
// 如果找到了则返回找到的第一个对应token的下标，否则返回值无意义。
template <Token token>
size_t FindTokenFromRange(const std::vector<WamonToken> &tokens, size_t begin, size_t end, bool &finded) {
  assert(tokens.size() >= end);
  assert(begin <= end);
  for (size_t i = begin; i <= end; ++i) {
    if (tokens[i].token == token) {
      finded = true;
      return i;
    }
  }
  finded = false;
  return 0;
}

class Type;
std::unique_ptr<Type> ParseType(const std::vector<WamonToken> &tokens, size_t &begin);

void ParseTypeList(const std::vector<WamonToken> &tokens, size_t begin, size_t end,
                   std::vector<std::unique_ptr<Type>> &param_list, std::unique_ptr<Type> &return_type);

std::unique_ptr<Statement> ParseStatement(PackageUnit &pu, const std::vector<WamonToken> &tokens, size_t begin,
                                          size_t &next);

std::vector<std::pair<std::string, std::unique_ptr<Type>>> ParseParameterList(const std::vector<WamonToken> &tokens,
                                                                              size_t begin, size_t end);

std::unique_ptr<FunctionDef> TryToParseFunctionDeclaration(PackageUnit &pu, const std::vector<WamonToken> &tokens,
                                                           size_t &begin);

std::unique_ptr<OperatorDef> TryToParseOperatorOverride(PackageUnit &pu, const std::vector<WamonToken> &tokens,
                                                        size_t &begin, Token &);

std::unique_ptr<StructDef> TryToParseStructDeclaration(const std::vector<WamonToken> &tokens, size_t &begin);

std::unique_ptr<VariableDefineStmt> TryToParseVariableDeclaration(PackageUnit &pu,
                                                                  const std::vector<WamonToken> &tokens, size_t &begin);

std::unique_ptr<BlockStmt> ParseStmtBlock(PackageUnit &pu, const std::vector<WamonToken> &tokens, size_t begin,
                                          size_t end);

std::vector<std::unique_ptr<Expression>> ParseExprList(PackageUnit &pu, const std::vector<WamonToken> &tokens,
                                                       size_t begin, size_t end);

std::unique_ptr<Expression> ParseExpression(PackageUnit &pu, const std::vector<WamonToken> &tokens, size_t begin,
                                            size_t end);

std::unique_ptr<Statement> TryToParseSkipStmt(PackageUnit &pu, const std::vector<WamonToken> &tokens, size_t begin,
                                              size_t &next);

std::unique_ptr<ExpressionStmt> ParseExprStmt(PackageUnit &pu, const std::vector<WamonToken> &tokens, size_t begin,
                                              size_t &next);

std::string ParsePackageName(const std::vector<WamonToken> &tokens, size_t &begin);

std::vector<std::string> ParseImportPackages(const std::vector<WamonToken> &tokens, size_t &begin);

PackageUnit Parse(const std::vector<WamonToken> &tokens);

}  // namespace wamon
