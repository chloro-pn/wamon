#include <string>

#include "gtest/gtest.h"
#include "wamon/inner_type_method.h"
#include "wamon/interpreter.h"
#include "wamon/package_unit.h"
#include "wamon/parser.h"
#include "wamon/scanner.h"
#include "wamon/type_checker.h"
#include "wamon/var_as.h"

TEST(builtin_type_method, basic) {
  auto builtintype = wamon::TypeFactory<std::string>::Get();
  auto return_type = wamon::InnerTypeMethod::Instance().CheckAndGetReturnType(builtintype, "len", {});
  EXPECT_EQ(return_type->GetTypeInfo(), "int");

  builtintype = wamon::TypeFactory<std::vector<double>>::Get();
  std::vector<std::unique_ptr<wamon::Type>> params;
  params.push_back(wamon::TypeFactory<int>::Get());
  return_type = wamon::InnerTypeMethod::Instance().CheckAndGetReturnType(builtintype, "at", params);
  EXPECT_EQ(return_type->GetTypeInfo(), "double");
}

TEST(builtin_type_method, list) {
  using namespace wamon;
  Scanner scan;
  std::string script = R"(
    package main;

    let v : list(int) = (2, 3, 4);
  )";
  auto tokens = scan.Scan(script);

  auto pu = Parse(tokens);
  pu = MergePackageUnits(std::move(pu));
  TypeChecker tc(pu);
  std::string reason;
  bool succ = tc.CheckAll(reason);
  EXPECT_EQ(succ, true) << reason;
  Interpreter ip(pu);
  auto v = ip.ExecExpression(tc, "main", "call v:empty()");
  EXPECT_EQ(v->GetTypeInfo(), "bool");
  EXPECT_EQ(VarAs<bool>(v), false);
  ip.ExecExpression(tc, "main", "call v:clear()");
  v = ip.ExecExpression(tc, "main", "call v:empty()");
  EXPECT_EQ(VarAs<bool>(v), true);
}
