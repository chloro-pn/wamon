#include "wamon/parser.h"

#include "gtest/gtest.h"
#include "wamon/scanner.h"

TEST(parser, basic) {
  wamon::Scanner scan;
  std::string str = "struct test [int a;]";
  auto tokens = scan.Scan(str);
  EXPECT_THROW(wamon::Parse(tokens), std::runtime_error);

  str = R"(
    struct my_struct_name {
      int a;
      double b;
      string c;
    }
  )";

  tokens = scan.Scan(str);
  EXPECT_NO_THROW(wamon::Parse(tokens));
}

TEST(parser, find_match) {
  wamon::Scanner scan;
  std::string str = "(func(2,3), func(()(())()), ax)";
  auto tokens = scan.Scan(str);
  auto end = wamon::FindMatchedToken<wamon::Token::LEFT_PARENTHESIS, wamon::Token::RIGHT_PARENTHESIS>(tokens, 0);
  EXPECT_EQ(end, tokens.size() - 2);
}

TEST(parser, find_next) {
  wamon::Scanner scan;
  std::string str = "(func(2,3), [,], func((,){,(),},()), ax)";
  auto tokens = scan.Scan(str);
  auto end = wamon::FindMatchedToken<wamon::Token::LEFT_PARENTHESIS, wamon::Token::RIGHT_PARENTHESIS>(tokens, 0);
  EXPECT_EQ(end, tokens.size() - 2);
  size_t begin = 0;
  std::vector<size_t> indexs = {7, 11, 27, 29};
  size_t count = 0;
  while (begin != end) {
    auto next = wamon::FindNextToken<wamon::Token::COMMA>(tokens, begin, end);
    EXPECT_EQ(next, indexs[count++]);
    begin = next;
  }
}

TEST(parser, parse_parameter_list) {
  wamon::Scanner scan;
  std::string str = "(int a, double b, string c)";
  auto tokens = scan.Scan(str);
  auto param_list = wamon::ParseParameterList(tokens, 0, tokens.size() - 2);
  EXPECT_EQ(param_list[0].first, "a");
  EXPECT_EQ(param_list[0].second->GetTypeInfo(), "int");
  EXPECT_EQ(param_list[1].first, "b");
  EXPECT_EQ(param_list[1].second->GetTypeInfo(), "double");
  EXPECT_EQ(param_list[2].first, "c");
  EXPECT_EQ(param_list[2].second->GetTypeInfo(), "string");
  EXPECT_EQ(param_list.size(), 3);
  str = "()";
  tokens = scan.Scan(str);
  param_list = wamon::ParseParameterList(tokens, 0, tokens.size() - 2);
  EXPECT_EQ(param_list.size(), 0);
}

TEST(parser, function_declaration) {
  wamon::Scanner scan;
  std::string str = "func myfunc(int a, double b, string c) -> void {}";
  auto tokens = scan.Scan(str);
  size_t begin = 0;
  wamon::TryToParseFunctionDeclaration(tokens, begin);
  EXPECT_EQ(tokens[begin].token, wamon::Token::TEOF);
}

TEST(parser, parse_stmt) {
  wamon::Scanner scan;
  std::string str = "if (a.b(c, d)) { call myfunc(b, c, d); } else { \"string_iter\"; }";
  auto tokens = scan.Scan(str);
  size_t next = 0;
  wamon::ParseStatement(tokens, 0, next);
  EXPECT_EQ(next, tokens.size() - 1);
}

TEST(parse, parse_expression) {
  wamon::Scanner scan;
  std::string str = "var_name.data_member";
  auto tokens = scan.Scan(str);
  auto expr = wamon::ParseExpression(tokens, 0, tokens.size() - 1);
  EXPECT_NE(expr, nullptr);
  str = "var_name.method(param_a, param_b, call myfunc(c, d))";
  tokens = scan.Scan(str);
  expr = wamon::ParseExpression(tokens, 0, tokens.size() - 1);
  EXPECT_NE(expr, nullptr);
}