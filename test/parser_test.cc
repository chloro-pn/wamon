#include "wamon/parser.h"

#include "gtest/gtest.h"
#include "wamon/enum_def.h"
#include "wamon/exception.h"
#include "wamon/package_unit.h"
#include "wamon/scanner.h"

TEST(parser, basic) {
  wamon::Scanner scan;
  std::string str = "struct test [int a;]";
  auto tokens = scan.Scan(str);
  EXPECT_THROW(wamon::Parse(tokens), wamon::WamonException);

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
  EXPECT_THROW(test_func(tokens), wamon::WamonException);
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
  wamon::PackageUnit pu;
  auto param_list = wamon::ParseParameterList(pu, tokens, 0, tokens.size() - 2);
  EXPECT_EQ(param_list[0].name, "$a");
  EXPECT_EQ(param_list[0].type->GetTypeInfo(), "int");
  EXPECT_EQ(param_list[1].name, "$b");
  EXPECT_EQ(param_list[1].type->GetTypeInfo(), "double");
  EXPECT_EQ(param_list[2].name, "$c");
  EXPECT_EQ(param_list[2].type->GetTypeInfo(), "string");
  EXPECT_EQ(param_list.size(), 3);
  str = "()";
  tokens = scan.Scan(str);
  param_list = wamon::ParseParameterList(pu, tokens, 0, tokens.size() - 2);
  EXPECT_EQ(param_list.size(), 0);
}

TEST(parser, function_declaration) {
  wamon::Scanner scan;
  std::string str = "func myfunc(int a, double b, string c) -> void {}";
  auto tokens = scan.Scan(str);
  size_t begin = 0;
  wamon::PackageUnit pu;
  wamon::TryToParseFunctionDeclaration(pu, tokens, begin);
  EXPECT_EQ(tokens[begin].token, wamon::Token::TEOF);
}

TEST(parser, operator_override) {
  wamon::Scanner scan;

  std::string str = R"(
    package main;

    struct my_struct_name {
      int a;
      double b;
      string c;
    }

    method my_struct_name {
      operator () (string x, double xx) -> int {
        if (x == self.c && xx == self.b) {
          return self.a;
        }
        return 0;
      }
    }

    func main() -> int {
      let m : my_struct_name = (2, 3.0, "bob");
      let tmp : int = (m + 1);
      return 0;
    }
  )";

  auto tokens = scan.Scan(str);
  EXPECT_NO_THROW(wamon::Parse(tokens));
}

TEST(parser, operator_priority) {
  wamon::Scanner scan;
  wamon::PackageUnit pu;
  std::string str = "a.b[2]";
  auto tokens = scan.Scan(str);
  auto expr = wamon::ParseExpression(pu, tokens, 0, tokens.size() - 2);
  auto be = dynamic_cast<wamon::BinaryExpr*>(expr.get());
  EXPECT_NE(be, nullptr);
  EXPECT_EQ(be->GetOp(), wamon::Token::SUBSCRIPT);
  auto left = dynamic_cast<wamon::BinaryExpr*>(be->GetLeft().get());
  EXPECT_NE(left, nullptr);
  EXPECT_EQ(left->GetOp(), wamon::Token::MEMBER_ACCESS);

  str = "call mf:(a, b)[4]";
  tokens = scan.Scan(str);
  EXPECT_NO_THROW(wamon::ParseExpression(pu, tokens, 0, tokens.size() - 2));
}

TEST(parser, struct_trait) {
  wamon::Scanner scan;
  std::string str = R"(
    package main;

    struct trait my_trait {
      int a;
      double b;
      string c;
    }

    method my_trait {
      operator () (string x, double xx) -> int;
      func get_a() -> int;
      func get_c() -> string;
    }
  )";
  auto tokens = scan.Scan(str);
  auto pu = wamon::Parse(tokens);
  EXPECT_EQ(pu.GetStructs().size(), 1);
  bool is_trait = pu.GetStructs().find("my_trait")->second->IsTrait();
  EXPECT_EQ(is_trait, true);
  const auto& methods_def = pu.GetStructs().find("my_trait")->second->GetMethods();
  EXPECT_EQ(methods_def.size(), 3);
  EXPECT_EQ(methods_def[1]->GetMethodName(), "get_a");
  EXPECT_EQ(methods_def[1]->IsDeclaration(), true);
}

TEST(parser, parse_stmt) {
  wamon::Scanner scan;
  wamon::PackageUnit pu;
  std::string str = "if (a.b) { call myfunc:(b, c, d[3]); break; } elif(true) { cc; } else { \"string_iter\"; }";
  auto tokens = scan.Scan(str);
  size_t next = 0;
  auto stmt = wamon::ParseStatement(pu, tokens, 0, next);
  EXPECT_EQ(next, tokens.size() - 1);
  EXPECT_EQ(stmt->GetStmtName(), "if_stmt");

  str = "{}";
  tokens = scan.Scan(str);
  next = wamon::FindMatchedToken<wamon::Token::LEFT_BRACE, wamon::Token::RIGHT_BRACE>(tokens, 0);
  stmt = wamon::ParseStmtBlock(pu, tokens, 0, next);
  next += 1;
  EXPECT_EQ(next, tokens.size() - 1);
  EXPECT_EQ(stmt->GetStmtName(), "block_stmt");

  str = "while(true) { call myfunc:(a, b, c); }";
  tokens = scan.Scan(str);
  next = 0;
  stmt = wamon::ParseStatement(pu, tokens, 0, next);
  EXPECT_EQ(next, tokens.size() - 1);
  EXPECT_EQ(stmt->GetStmtName(), "while_stmt");

  str = "let var : string = (\"hello world\");";
  tokens = scan.Scan(str);
  next = 0;
  stmt = wamon::ParseStatement(pu, tokens, 0, next);
  EXPECT_EQ(next, tokens.size() - 1);
  EXPECT_EQ(stmt->GetStmtName(), "var_def_stmt");

  str = "let var : list(ptr(string)) = (\"hello world\");";
  tokens = scan.Scan(str);
  next = 0;
  stmt = wamon::ParseStatement(pu, tokens, 0, next);
  EXPECT_EQ(next, tokens.size() - 1);
  EXPECT_EQ(stmt->GetStmtName(), "var_def_stmt");

  str = "continue;";
  tokens = scan.Scan(str);
  next = 0;
  stmt = wamon::TryToParseSkipStmt(pu, tokens, 0, next);
  EXPECT_EQ(next, tokens.size() - 1);
  EXPECT_GE(stmt->GetStmtName(), "continue_stmt");

  str = "return call my_func:();";
  tokens = scan.Scan(str);
  next = 0;
  stmt = wamon::TryToParseSkipStmt(pu, tokens, 0, next);
  EXPECT_EQ(next, tokens.size() - 1);
  EXPECT_EQ(stmt->GetStmtName(), "return_stmt");

  str = "marr[call get_arr_index:()];";
  tokens = scan.Scan(str);
  next = 0;
  stmt = wamon::ParseExprStmt(pu, tokens, 0, next);
  EXPECT_EQ(next, tokens.size() - 1);
  EXPECT_EQ(stmt->GetStmtName(), "expr_stmt");
}

TEST(parse, parse_expression) {
  wamon::Scanner scan;
  wamon::PackageUnit pu;
  std::string str = "var_name.data_member";
  auto tokens = scan.Scan(str);
  auto expr = wamon::ParseExpression(pu, tokens, 0, tokens.size() - 1);
  EXPECT_NE(expr, nullptr);
  str = "range | view";
  tokens = scan.Scan(str);
  expr = wamon::ParseExpression(pu, tokens, 0, tokens.size() - 1);
  EXPECT_NE(expr, nullptr);
  EXPECT_NE(dynamic_cast<wamon::BinaryExpr*>(expr.get()), nullptr);
  auto ptr = dynamic_cast<wamon::BinaryExpr*>(expr.get());
  EXPECT_EQ(ptr->GetOp(), wamon::Token::PIPE);

  str = "new int(2)";
  tokens = scan.Scan(str);
  expr = wamon::ParseExpression(pu, tokens, 0, tokens.size() - 1);
  EXPECT_NE(expr, nullptr);
  auto ptr2 = dynamic_cast<wamon::NewExpr*>(expr.get());
  EXPECT_EQ(ptr2->GetNewType()->GetTypeInfo(), "int");
  EXPECT_EQ(ptr2->GetParameters().size(), 1);

  str = "alloc int(2)";
  tokens = scan.Scan(str);
  expr = wamon::ParseExpression(pu, tokens, 0, tokens.size() - 1);
  EXPECT_NE(expr, nullptr);
  auto ptr3 = dynamic_cast<wamon::AllocExpr*>(expr.get());
  EXPECT_EQ(ptr3->GetAllocType()->GetTypeInfo(), "int");
  EXPECT_EQ(ptr3->GetParameters().size(), 1);

  str = "dealloc ptr_id";
  tokens = scan.Scan(str);
  expr = wamon::ParseExpression(pu, tokens, 0, tokens.size() - 1);
  EXPECT_NE(expr, nullptr);
  auto ptr4 = dynamic_cast<wamon::DeallocExpr*>(expr.get());
  auto id = dynamic_cast<wamon::IdExpr*>(ptr4->GetDeallocParam().get())->GetId();
  EXPECT_EQ(id, "ptr_id");
}

TEST(parse, unary_operator) {
  wamon::Scanner scan;
  wamon::PackageUnit pu;
  std::string str = "a + -&b";
  auto tokens = scan.Scan(str);
  auto expr = wamon::ParseExpression(pu, tokens, 0, tokens.size() - 1);
  EXPECT_NE(expr, nullptr);
  auto tmp = dynamic_cast<wamon::BinaryExpr*>(expr.get())->GetRight().get();
  auto tmp2 = dynamic_cast<wamon::UnaryExpr*>(tmp);
  EXPECT_NE(tmp2, nullptr);
  EXPECT_EQ(tmp2->GetOp(), wamon::Token::MINUS);
  tmp2 = dynamic_cast<wamon::UnaryExpr*>(tmp2->GetOperand().get());
  EXPECT_EQ(tmp2->GetOp(), wamon::Token::ADDRESS_OF);
}

TEST(parse, parse_package) {
  wamon::Scanner scan;
  std::string str = "package main; import net; import os;";
  auto tokens = scan.Scan(str);
  size_t begin = 0;
  wamon::PackageUnit pu;
  std::string package_name = wamon::ParsePackageName(pu, tokens, begin);
  auto imports = wamon::ParseImportPackages(pu, tokens, begin);
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
  wamon::PackageUnit pu;
  auto type = wamon::ParseType(pu, tokens, begin);
  EXPECT_EQ(type->GetTypeInfo(), "string");
  EXPECT_EQ(begin, tokens.size() - 1);

  str = "mystruct";
  tokens = scan.Scan(str);
  begin = 0;
  type = wamon::ParseType(pu, tokens, begin);
  EXPECT_EQ(type->GetTypeInfo(), "mystruct");
  EXPECT_EQ(begin, tokens.size() - 1);

  str = "ptr(int)";
  tokens = scan.Scan(str);
  begin = 0;
  type = wamon::ParseType(pu, tokens, begin);
  EXPECT_EQ(type->GetTypeInfo(), "ptr(int)");
  EXPECT_EQ(begin, tokens.size() - 1);
  EXPECT_EQ(type->IsBasicType(), false);

  str = "list(int)";
  tokens = scan.Scan(str);
  begin = 0;
  type = wamon::ParseType(pu, tokens, begin);
  EXPECT_EQ(type->GetTypeInfo(), "list(int)");
  EXPECT_EQ(begin, tokens.size() - 1);
  EXPECT_EQ(type->IsBasicType(), false);

  str = "list(ptr(int))";
  tokens = scan.Scan(str);
  begin = 0;
  type = wamon::ParseType(pu, tokens, begin);
  EXPECT_EQ(type->GetTypeInfo(), "list(ptr(int))");
  EXPECT_EQ(begin, tokens.size() - 1);
  EXPECT_EQ(type->IsBasicType(), false);

  str = "f((int, string) -> double)";
  tokens = scan.Scan(str);
  begin = 0;
  type = wamon::ParseType(pu, tokens, begin);
  EXPECT_EQ(type->GetTypeInfo(), "f((int, string) -> double)");
  EXPECT_EQ(begin, tokens.size() - 1);
  EXPECT_EQ(type->IsBasicType(), false);

  str = "f(() -> double)";
  tokens = scan.Scan(str);
  begin = 0;
  type = wamon::ParseType(pu, tokens, begin);
  EXPECT_EQ(type->GetTypeInfo(), "f(() -> double)");
  EXPECT_EQ(begin, tokens.size() - 1);
  EXPECT_EQ(type->IsBasicType(), false);

  str = "f(())";
  tokens = scan.Scan(str);
  begin = 0;
  type = wamon::ParseType(pu, tokens, begin);
  EXPECT_EQ(type->GetTypeInfo(), "f(() -> void)");
  EXPECT_EQ(begin, tokens.size() - 1);
  EXPECT_EQ(type->IsBasicType(), false);

  str = "f((int, mystruct, ptr(int), list(ptr(double))) -> void)";
  tokens = scan.Scan(str);
  begin = 0;
  type = wamon::ParseType(pu, tokens, begin);
  EXPECT_EQ(type->GetTypeInfo(), "f((int, mystruct, ptr(int), list(ptr(double))) -> void)");
  EXPECT_EQ(begin, tokens.size() - 1);
  EXPECT_EQ(type->IsBasicType(), false);
}

TEST(parse, parse_enum) {
  wamon::Scanner scan;
  std::string str = R"(
    enum Animal {
      cat;
      dog;
      bird;
    }
  )";
  auto tokens = scan.Scan(str);
  size_t begin = 0;
  wamon::PackageUnit pu;
  auto enum_def = wamon::TryToParseEnumDeclaration(pu, tokens, begin);
  EXPECT_NE(enum_def, nullptr);
  EXPECT_EQ(enum_def->GetEnumName(), "Animal");
  EXPECT_EQ(enum_def->GetEnumItems().size(), 3);
  EXPECT_EQ(enum_def->GetEnumItems()[0], "cat");
  EXPECT_EQ(begin, tokens.size() - 1);
}