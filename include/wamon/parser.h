#pragma once

#include "wamon/ast.h"
#include "wamon/scanner.h"

namespace wamon {

void AssertTokenOrThrow(const std::vector<WamonToken> &tokens, size_t &begin, Token token);

template<Token left, Token right>
size_t FindMatchedToken(const std::vector<WamonToken> &tokens, size_t begin) {
  AssertTokenOrThrow(tokens, begin, left);
  size_t counter = 1;
  size_t index = begin;
  while(index < tokens.size()) {
    if (tokens[index].token == left) {
      ++counter;
    }
    else if (tokens[index].token == right) {
      --counter;
      if (counter == 0) {
        return index;
      }
    }
    ++index;
  }
  throw std::runtime_error("find matched parenthesis error");
}

template <Token token>
size_t FindNextToken(const std::vector<WamonToken> &tokens, size_t begin, size_t end) {
  if (tokens.size() <= end || tokens[end].token == token) {
    throw std::runtime_error("find next token error");
  }
  begin += 1;
  size_t counter1 = 0; // ()
  size_t counter2 = 0; // []
  size_t counter3 = 0; // {}
  for(size_t i = begin; i <= end; ++i) {
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

std::vector<std::pair<std::string, std::unique_ptr<Type>>> ParseParameterList(const std::vector<WamonToken>& tokens, size_t begin, size_t end);

void TryToParseFunctionDeclaration(const std::vector<WamonToken> &tokens, size_t& begin);

void TryToParseStructDeclaration(const std::vector<WamonToken> &tokens, size_t& begin);

void TryToParseVariableDeclaration(const std::vector<WamonToken> &tokens, size_t& begin);

std::vector<std::unique_ptr<Statement>> ParseStmtBlock(const std::vector<WamonToken>& tokens, size_t begin, size_t end);

void Parse(const std::vector<WamonToken> &tokens);

}  // namespace wamon