#include "wamon/scanner.h"

#include "gtest/gtest.h"

TEST(scanner, basic) {
  std::string str(R"(
    if ( a > 3) { 
      a = 4;
    }
  )");
  wamon::Scanner scan;
  auto tokens = scan.Scan(str);
  EXPECT_EQ(tokens[0].token, wamon::Token::IF);
  EXPECT_EQ(tokens[1].token, wamon::Token::LEFT_PARENTHESIS);
  EXPECT_EQ(tokens[2].token, wamon::Token::ID);
  EXPECT_EQ(tokens[2].Get<std::string>(), "a");
  EXPECT_EQ(tokens[3].token, wamon::Token::GT);
  EXPECT_EQ(tokens[4].token, wamon::Token::INT_ITERAL);
  EXPECT_EQ(tokens[5].token, wamon::Token::RIGHT_PARENTHESIS);
  EXPECT_EQ(tokens[6].token, wamon::Token::LEFT_BRACE);
  EXPECT_EQ(tokens[7].token, wamon::Token::ID);
  EXPECT_EQ(tokens[8].token, wamon::Token::ASSIGN);
  EXPECT_EQ(tokens[9].token, wamon::Token::INT_ITERAL);
  EXPECT_EQ(tokens[10].token, wamon::Token::SEMICOLON);
  EXPECT_EQ(tokens[11].token, wamon::Token::RIGHT_BRACE);
  EXPECT_EQ(tokens[12].token, wamon::Token::TEOF);
  EXPECT_EQ(tokens.size(), 13);
}

TEST(scanner, str) {
  std::string str("\"hello world\"");
  wamon::Scanner scan;
  auto tokens = scan.Scan(str);
  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0].token, wamon::Token::STRING_ITERAL);
}

TEST(scanner, int_) {
  std::string str("-232");
  wamon::Scanner scan;
  auto tokens = scan.Scan(str);
  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0].token, wamon::Token::INT_ITERAL);
}

TEST(scanner, byte) {
  std::string str("0XAB");
  wamon::Scanner scan;
  auto tokens = scan.Scan(str);
  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0].token, wamon::Token::BYTE_ITERAL);
}

TEST(scanner, double_) {
  std::string str("+2.143");
  wamon::Scanner scan;
  auto tokens = scan.Scan(str);
  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0].token, wamon::Token::DOUBLE_ITERAL);
}

TEST(scanner, logical) {
  std::string str("&&");
  wamon::Scanner scan;
  auto tokens = scan.Scan(str);
  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0].token, wamon::Token::AND);

  str = "||";
  tokens = scan.Scan(str);
  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0].token, wamon::Token::OR);

  str = "!";
  tokens = scan.Scan(str);
  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0].token, wamon::Token::NOT);
}

TEST(scanner, pipe) {
  std::string str("range | view");
  wamon::Scanner scan;
  auto tokens = scan.Scan(str);
  EXPECT_EQ(tokens.size(), 4);
  EXPECT_EQ(tokens[1].token, wamon::Token::PIPE);
}
