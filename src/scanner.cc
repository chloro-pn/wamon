#include "wamon/scanner.h"

#include <regex>
#include <sstream>

#include "wamon/exception.h"
#include "wamon/key_words.h"
#include "wamon/reserved_id.h"

namespace wamon {

static uint8_t string_to_byte(const std::string &str) {
  if (str.size() != 4 || str[0] != '0' || str[1] != 'X') {
    throw WamonException("invalid format (byte) : {}", str);
  }
  int b1 = 0, b2 = 0;
  if (str[2] >= '0' && str[2] <= '9') {
    b1 = str[2] - '0';
  } else if (str[2] >= 'A' && str[2] <= 'F') {
    b1 = str[2] - 'A' + 10;
  } else {
    throw WamonException("invalid format (byte) : {}", str);
  }
  if (str[3] >= '0' && str[3] <= '9') {
    b2 = str[3] - '0';
  } else if (str[3] >= 'A' && str[3] <= 'F') {
    b2 = str[3] - 'A' + 10;
  } else {
    throw WamonException("invalid format (byte) : {}", str);
  }
  return static_cast<uint8_t>(b1 * 16 + b2);
}

void Scanner::scan(const std::string &str, std::vector<WamonToken> &tokens) {
  auto begin = str.cbegin();
  auto end = str.cend();
  while (true) {
    std::smatch result;
    if (std::regex_search(begin, end, result, std::regex("[ \\r\\n\\t]"), std::regex_constants::match_continuous)) {
      ++begin;
    }
    // 单行注释
    else if (std::regex_search(begin, end, result, std::regex("//.*?\\n"), std::regex_constants::match_continuous)) {
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex("[+-]?(0|[1-9][0-9]*)\\.[0-9]*"),
                                 std::regex_constants::match_continuous)) {
      double num;
      std::stringstream stream;
      stream << result[0].str();
      stream >> num;
      tokens.push_back(WamonToken(Token::DOUBLE_ITERAL, num));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex("0X[0-9A-F][0-9A-F]"),
                                 std::regex_constants::match_continuous)) {
      uint8_t byte = string_to_byte(result[0].str());
      tokens.push_back(WamonToken(Token::BYTE_ITERAL, byte));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex("[+-]?(0|[1-9][0-9]*)"),
                                 std::regex_constants::match_continuous)) {
      int64_t num;
      std::stringstream stream;
      stream << result[0].str();
      stream >> num;
      tokens.push_back(WamonToken(Token::INT_ITERAL, num));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex("[a-zA-Z_][a-zA-Z0-9_]*"),
                                 std::regex_constants::match_continuous)) {
      const std::string &id = result[0].str();
      // 关键字 or 自定义标识符
      if (KeyWords::Instance().Find(id) == true) {
        tokens.push_back(WamonToken(KeyWords::Instance().Get(id), id));
      } else {
        if (IsReservedId(id)) {
          throw WamonException("Scanner::scan error, reserved id {}", id);
        }
        tokens.push_back(WamonToken(Token::ID, id));
      }
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex("@"), std::regex_constants::match_continuous)) {
      tokens.push_back(WamonToken(Token::CALL));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex("\\("), std::regex_constants::match_continuous)) {
      tokens.push_back(WamonToken(Token::LEFT_PARENTHESIS));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex("\\)"), std::regex_constants::match_continuous)) {
      tokens.push_back(WamonToken(Token::RIGHT_PARENTHESIS));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex("\\["), std::regex_constants::match_continuous)) {
      tokens.push_back(WamonToken(Token::LEFT_BRACKETS));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex("\\]"), std::regex_constants::match_continuous)) {
      tokens.push_back(WamonToken(Token::RIGHT_BRACKETS));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex("\\{"), std::regex_constants::match_continuous)) {
      tokens.push_back(WamonToken(Token::LEFT_BRACE));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex("\\}"), std::regex_constants::match_continuous)) {
      tokens.push_back(WamonToken(Token::RIGHT_BRACE));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex(";"), std::regex_constants::match_continuous)) {
      tokens.push_back(WamonToken(Token::SEMICOLON));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex("::"), std::regex_constants::match_continuous)) {
      tokens.push_back(WamonToken(Token::SCOPE));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex(":"), std::regex_constants::match_continuous)) {
      tokens.push_back(WamonToken(Token::COLON));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex(","), std::regex_constants::match_continuous)) {
      tokens.push_back(WamonToken(Token::COMMA));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex("\\+\\+"), std::regex_constants::match_continuous)) {
      tokens.push_back(WamonToken(Token::INCREMENT));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex("\\+"), std::regex_constants::match_continuous)) {
      tokens.push_back(WamonToken(Token::PLUS));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex("->"), std::regex_constants::match_continuous)) {
      tokens.push_back(WamonToken(Token::ARROW));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex("-"), std::regex_constants::match_continuous)) {
      tokens.push_back(WamonToken(Token::MINUS));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex("\\*"), std::regex_constants::match_continuous)) {
      tokens.push_back(WamonToken(Token::MULTIPLY));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex("/"), std::regex_constants::match_continuous)) {
      tokens.push_back(WamonToken(Token::DIVIDE));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex("&&"), std::regex_constants::match_continuous)) {
      tokens.push_back(WamonToken(Token::AND));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex("\\|\\|"), std::regex_constants::match_continuous)) {
      tokens.push_back(WamonToken(Token::OR));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex("!"), std::regex_constants::match_continuous)) {
      tokens.push_back(WamonToken(Token::NOT));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex("&"), std::regex_constants::match_continuous)) {
      tokens.push_back(WamonToken(Token::ADDRESS_OF));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex("\\|"), std::regex_constants::match_continuous)) {
      tokens.push_back(WamonToken(Token::PIPE));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex("\\."), std::regex_constants::match_continuous)) {
      tokens.push_back(WamonToken(Token::MEMBER_ACCESS));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex("=="), std::regex_constants::match_continuous)) {
      tokens.push_back(WamonToken(Token::COMPARE));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex("="), std::regex_constants::match_continuous)) {
      tokens.push_back(WamonToken(Token::ASSIGN));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex(">="), std::regex_constants::match_continuous)) {
      tokens.push_back(WamonToken(Token::GTE));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex(">"), std::regex_constants::match_continuous)) {
      tokens.push_back(WamonToken(Token::GT));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex("<="), std::regex_constants::match_continuous)) {
      tokens.push_back(WamonToken(Token::LTE));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex("<"), std::regex_constants::match_continuous)) {
      tokens.push_back(WamonToken(Token::LT));
      begin = result[0].second;
    } else if (std::regex_search(begin, end, result, std::regex("\"(.*?)\""), std::regex_constants::match_continuous)) {
      tokens.push_back(WamonToken(Token::STRING_ITERAL, result[1].str()));
      begin = result[0].second;
    } else {
      // 无法识别为合法的token
      throw WamonException("scan error");
    }
    if (begin == end) {
      break;
    }
  }
}

}  // namespace wamon
