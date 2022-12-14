#include "wamon/parser.h"

#include "gtest/gtest.h"
#include "wamon/scanner.h"

TEST(parser, basic) {
  wamon::Scanner scan;
  std::string str = "struct test [int a;]";
  auto tokens = scan.Scan(str);
  EXPECT_THROW(wamon::Parse(tokens), std::runtime_error);

  str = R"(
    package main;

    struct my_struct_name {
      int a;
      double b;
      string c;
    }
  )";

  tokens = scan.Scan(str);
  EXPECT_NO_THROW(wamon::Parse(tokens));
}

void test_func(const std::vector<wamon::WamonToken>& tokens) {
  std::string str = "{{{";
  wamon::FindMatchedToken<wamon::Token::LEFT_PARENTHESIS, wamon::Token::RIGHT_PARENTHESIS>(tokens, 0);
}

TEST(parser, find_match) {
  wamon::Scanner scan;
  std::string str = "(func(2,3), func(()(())()), ax)";
  auto tokens = scan.Scan(str);
  auto end = wamon::FindMatchedToken<wamon::Token::LEFT_PARENTHESIS, wamon::Token::RIGHT_PARENTHESIS>(tokens, 0);
  EXPECT_EQ(end, tokens.size() - 2);

  str = "{{{";
  tokens = scan.Scan(str);
  EXPECT_THROW(test_func(tokens), std::runtime_error);
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
  std::string str = "if (a.b(c, d)) { call myfunc(b, c, d[3]); break; } else { \"string_iter\"; }";
  auto tokens = scan.Scan(str);
  size_t next = 0;
  auto stmt = wamon::ParseStatement(tokens, 0, next);
  EXPECT_EQ(next, tokens.size() - 1);
  EXPECT_EQ(stmt->GetStmtName(), "if_stmt");

  str = "{}";
  tokens = scan.Scan(str);
  next = wamon::FindMatchedToken<wamon::Token::LEFT_BRACE, wamon::Token::RIGHT_BRACE>(tokens, 0);
  stmt = wamon::ParseStmtBlock(tokens, 0, next);
  next += 1;
  EXPECT_EQ(next, tokens.size() - 1);
  EXPECT_EQ(stmt->GetStmtName(), "block_stmt");

  str = "while(true) { call myfunc(a, b, c); }";
  tokens = scan.Scan(str);
  next = 0;
  stmt = wamon::ParseStatement(tokens, 0, next);
  EXPECT_EQ(next, tokens.size() - 1);
  EXPECT_EQ(stmt->GetStmtName(), "while_stmt");

  str = "let var : string = (\"hello world\");";
  tokens = scan.Scan(str);
  next = 0;
  stmt = wamon::ParseStatement(tokens, 0, next);
  EXPECT_EQ(next, tokens.size() - 1);
  EXPECT_EQ(stmt->GetStmtName(), "var_def_stmt");

  str = "continue;";
  tokens = scan.Scan(str);
  next = 0;
  stmt = wamon::TryToParseSkipStmt(tokens, 0, next);
  EXPECT_EQ(next, tokens.size() - 1);
  EXPECT_GE(stmt->GetStmtName(), "continue_stmt");

  str = "return call my_func();";
  tokens = scan.Scan(str);
  next = 0;
  stmt = wamon::TryToParseSkipStmt(tokens, 0, next);
  EXPECT_EQ(next, tokens.size() - 1);
  EXPECT_EQ(stmt->GetStmtName(), "return_stmt");

  str = "marr[call get_arr_index()];";
  tokens = scan.Scan(str);
  next = 0;
  stmt = wamon::ParseExprStmt(tokens, 0, next);
  EXPECT_EQ(next, tokens.size() - 1);
  EXPECT_EQ(stmt->GetStmtName(), "expr_stmt");
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

TEST(parse, parse_package) {
  wamon::Scanner scan;
  std::string str = "package main; import net; import os;";
  auto tokens = scan.Scan(str);
  size_t begin = 0;
  std::string package_name = wamon::ParsePackageName(tokens, begin);
  auto imports = wamon::ParseImportPackages(tokens, begin);
  EXPECT_EQ(package_name, "main");
  EXPECT_EQ(imports.size(), 2);
  EXPECT_EQ(imports[0], "net");
  EXPECT_EQ(imports[1], "os");
}

TEST(parse, parse_type) {
  wamon::Scanner scan;
  std::string str = "string";
  auto tokens = scan.Scan(str);
  size_t begin = 0;
  auto type = wamon::ParseType(tokens, begin);
  EXPECT_EQ(type->GetTypeInfo(), "string");
  EXPECT_EQ(begin, tokens.size() - 1);

  str = "mystruct";
  tokens = scan.Scan(str);
  begin = 0;
  type = wamon::ParseType(tokens, begin);
  EXPECT_EQ(type->GetTypeInfo(), "mystruct");
  EXPECT_EQ(begin, tokens.size() - 1);

  str = "ptr(int)";
  tokens = scan.Scan(str);
  begin = 0;
  type = wamon::ParseType(tokens, begin);
  EXPECT_EQ(type->GetTypeInfo(), "ptr(int)");
  EXPECT_EQ(begin, tokens.size() - 1);
  EXPECT_EQ(type->IsBasicType(), false);

  str = "array(int, 3)";
  tokens = scan.Scan(str);
  begin = 0;
  type = wamon::ParseType(tokens, begin);
  EXPECT_EQ(type->GetTypeInfo(), "array(int)");
  EXPECT_EQ(begin, tokens.size() - 1);
  EXPECT_EQ(type->IsBasicType(), false);

  str = "array(ptr(int), 3)";
  tokens = scan.Scan(str);
  begin = 0;
  type = wamon::ParseType(tokens, begin);
  EXPECT_EQ(type->GetTypeInfo(), "array(ptr(int))");
  EXPECT_EQ(begin, tokens.size() - 1);
  EXPECT_EQ(type->IsBasicType(), false);

  str = "f((int, string) -> double)";
  tokens = scan.Scan(str);
  begin = 0;
  type = wamon::ParseType(tokens, begin);
  EXPECT_EQ(type->GetTypeInfo(), "f((int, string) -> double)");
  EXPECT_EQ(begin, tokens.size() - 1);
  EXPECT_EQ(type->IsBasicType(), false);

  str = "f(() -> double)";
  tokens = scan.Scan(str);
  begin = 0;
  type = wamon::ParseType(tokens, begin);
  EXPECT_EQ(type->GetTypeInfo(), "f(() -> double)");
  EXPECT_EQ(begin, tokens.size() - 1);
  EXPECT_EQ(type->IsBasicType(), false);

  str = "f(())";
  tokens = scan.Scan(str);
  begin = 0;
  type = wamon::ParseType(tokens, begin);
  EXPECT_EQ(type->GetTypeInfo(), "f(())");
  EXPECT_EQ(begin, tokens.size() - 1);
  EXPECT_EQ(type->IsBasicType(), false);

  str = "f((int, mystruct, ptr(int), array(ptr(double), 3)) -> void)";
  tokens = scan.Scan(str);
  begin = 0;
  type = wamon::ParseType(tokens, begin);
  EXPECT_EQ(type->GetTypeInfo(), "f((int, mystruct, ptr(int), array(ptr(double))) -> void)");
  EXPECT_EQ(begin, tokens.size() - 1);
  EXPECT_EQ(type->IsBasicType(), false);
}