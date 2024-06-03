#pragma once

#include <any>
#include <iostream>
#include <sstream>
#include <vector>

#include "wamon/token.h"

namespace wamon {

/*
 * @brief 词法分析产生的单元，由Token标识和区分同一类型token的值组成
 */
struct WamonToken {
  Token token;
  enum class ValueType { STR, INT, DOUBLE, BYTE, BOOL, NONE };
  ValueType type;
  std::any value;

  WamonToken() : token(Token::INVALID), type(ValueType::NONE), value() {}

  explicit WamonToken(Token token) : token(token), type(ValueType::NONE), value() {}

  WamonToken(Token token, const std::string &str) : token(token), type(ValueType::STR), value(str) {}

  WamonToken(Token token, double num) : token(token), type(ValueType::DOUBLE), value(num) {}

  WamonToken(Token token, uint8_t num) : token(token), type(ValueType::BYTE), value(num) {}

  WamonToken(Token token, int64_t num) : token(token), type(ValueType::INT), value(num) {}

  template <typename T>
  T Get() const {
    return std::any_cast<T>(value);
  }
};

class Scanner {
 public:
  Scanner() = default;

  const std::vector<WamonToken> &Scan(const std::string &str) {
    tokens_.clear();
    scan(str, tokens_);
    tokens_.push_back(WamonToken(Token::TEOF));
    return tokens_;
  }

  void PrintTokens() {
    size_t index = 0;
    for (auto &each : tokens_) {
      std::cout << index << " : " << GetTokenStr(each.token) << std::endl;
      index += 1;
    }
  }

 private:
  std::vector<WamonToken> tokens_;

  void scan(const std::string &str, std::vector<WamonToken> &tokens);
};

}  // namespace wamon
