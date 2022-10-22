#pragma once

#include "wamon/ast.h"
#include "wamon/scanner.h"
#include "wamon/struct_def.h"
#include "wamon/function_def.h"

namespace wamon {

void AssertTokenOrThrow(const std::vector<WamonToken> &tokens, size_t &begin, Token token);

template <Token left, Token right>
size_t FindMatchedToken(const std::vector<WamonToken> &tokens, size_t begin) {
  AssertTokenOrThrow(tokens, begin, left);
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
  throw std::runtime_error("find matched parenthesis error");
}

// 在区间(begin, end]中找到第一个值为token的token，注意需要跳过所有不匹配的大中小括号，如果找不到的话，返回end。
template <Token token>
size_t FindNextToken(const std::vector<WamonToken> &tokens, size_t begin,
                     size_t end = std::numeric_limits<size_t>::max()) {
  if (end != std::numeric_limits<size_t>::max()) {
    if (tokens.size() <= end || tokens[end].token == token) {
      throw std::runtime_error("find next token error");
    }
  }
  begin += 1;
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
  return end;
}

std::unique_ptr<Statement> ParseStatement(const std::vector<WamonToken> &tokens, size_t begin, size_t &next);

std::vector<std::pair<std::string, std::unique_ptr<Type>>> ParseParameterList(const std::vector<WamonToken> &tokens,
                                                                              size_t begin, size_t end);

std::unique_ptr<FunctionDef> TryToParseFunctionDeclaration(const std::vector<WamonToken> &tokens, size_t &begin);

std::unique_ptr<StructDef> TryToParseStructDeclaration(const std::vector<WamonToken> &tokens, size_t &begin);

std::unique_ptr<VariableDefineStmt> TryToParseVariableDeclaration(const std::vector<WamonToken> &tokens, size_t &begin);

std::vector<std::unique_ptr<Statement>> ParseStmtBlock(const std::vector<WamonToken> &tokens, size_t begin, size_t end);

std::vector<std::unique_ptr<Expression>> ParseExprList(const std::vector<WamonToken> &tokens, size_t begin, size_t end);

std::unique_ptr<Expression> ParseExpression(const std::vector<WamonToken> &tokens, size_t begin, size_t end);

std::unique_ptr<Statement> TryToParseSkipStmt(const std::vector<WamonToken>& tokens, size_t begin, size_t &next);

std::string ParsePackageName(const std::vector<WamonToken>& tokens, size_t &begin);

std::vector<std::string> ParseImportPackages(const std::vector<WamonToken>& tokens, size_t& begin);

void Parse(const std::vector<WamonToken> &tokens);

}  // namespace wamon