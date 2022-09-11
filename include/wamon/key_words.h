#pragma once

#include <string>
#include <unordered_map>

#include "wamon/token.h"

namespace wamon {

class KeyWords {
 public:
  static KeyWords &Instance() {
    static KeyWords kw;
    return kw;
  }

  bool Find(const std::string &id) const { return key_words_.count(id) > 0; }

  Token Get(const std::string &id) { return key_words_.at(id); }

 private:
  std::unordered_map<std::string, Token> key_words_;

  KeyWords();
};

}  // namespace wamon