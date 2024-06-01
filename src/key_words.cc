#include "wamon/key_words.h"

namespace wamon {

static void register_buildin_keywords(std::unordered_map<std::string, Token> &kws) {
  kws["if"] = Token::IF;
  kws["else"] = Token::ELSE;
  kws["elif"] = Token::ELIF;
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
  kws["trait"] = Token::TRAIT;
  kws["let"] = Token::LET;
  kws["ref"] = Token::REF;
  kws["new"] = Token::NEW;
  kws["alloc"] = Token::ALLOC;
  kws["dealloc"] = Token::DEALLOC;
  kws["method"] = Token::METHOD;
  kws["self"] = Token::SELF;
  kws["ptr"] = Token::PTR;
  kws["list"] = Token::LIST;
  kws["f"] = Token::F;
  kws["move"] = Token::MOVE;
  kws["package"] = Token::PACKAGE;
  kws["import"] = Token::IMPORT;
  kws["as"] = Token::AS;
  kws["lambda"] = Token::LAMBDA;
}

KeyWords::KeyWords() { register_buildin_keywords(key_words_); }

}  // namespace wamon
