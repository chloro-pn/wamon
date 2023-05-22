#include "wamon/key_words.h"

namespace wamon {

static void register_buildin_keywords(std::unordered_map<std::string, Token> &kws) {
  kws["if"] = Token::IF;
  kws["else"] = Token::ELSE;
  kws["while"] = Token::WHILE;
  kws["for"] = Token::FOR;
  kws["continue"] = Token::CONTINUE;
  kws["break"] = Token::BREAK;
  kws["return"] = Token::RETURN;
  kws["true"] = Token::TRUE;
  kws["false"] = Token::FALSE;
  kws["string"] = Token::STRING;
  kws["int"] = Token::INT;
  kws["double"] = Token::DOUBLE;
  kws["bool"] = Token::BOOL;
  kws["byte"] = Token::BYTE;
  kws["void"] = Token::VOID;
  kws["call"] = Token::CALL;
  kws["func"] = Token::FUNC;
  kws["operator"] = Token::OPERATOR;
  kws["struct"] = Token::STRUCT;
  kws["let"] = Token::LET;
  kws["method"] = Token::METHOD;
  kws["self"] = Token::SELF;
  kws["ptr"] = Token::PTR;
  kws["array"] = Token::ARRAY;
  kws["f"] = Token::F;
  kws["package"] = Token::PACKAGE;
  kws["import"] = Token::IMPORT;
}

KeyWords::KeyWords() { register_buildin_keywords(key_words_); }

}  // namespace wamon
