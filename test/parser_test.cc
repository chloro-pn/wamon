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
  auto end = wamon::FindMatchedParenthesis(tokens, 0);
  EXPECT_EQ(end, tokens.size() - 2);
}

TEST(parser, find_next) {
  wamon::Scanner scan;
  std::string str = "(func(2,3), [,], func((,){,(),},()), ax)";
  auto tokens = scan.Scan(str);
  auto end = wamon::FindMatchedParenthesis(tokens, 0);
  EXPECT_EQ(end, tokens.size() - 2);
  size_t begin = 0;
  std::vector<size_t> indexs = {7, 11, 27, 29};
  size_t count = 0;
  while(begin != end) {
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