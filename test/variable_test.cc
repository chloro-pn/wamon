#include "wamon/variable.h"

#include "gtest/gtest.h"
#include "wamon/interpreter.h"
#include "wamon/parser.h"
#include "wamon/scanner.h"
#include "wamon/static_analyzer.h"
#include "wamon/type_checker.h"

TEST(variable, list) {
  wamon::Scanner scan;
  std::string str = R"(
    package main;

    let a : list(int) = (2, 3, 4);

    func my_func() -> void {
      call a:insert(3, 5);
      return;
    }
  )";
  wamon::PackageUnit pu;
  auto tokens = scan.Scan(str);
  pu = wamon::Parse(tokens);
  pu = wamon::MergePackageUnits(std::move(pu));

  wamon::TypeChecker tc(pu);
  std::string reason;
  bool succ = tc.CheckAll(reason);
  EXPECT_EQ(succ, true);
  wamon::Interpreter interpreter(pu);
  auto v = interpreter.CallFunctionByName("main$my_func", {});
  EXPECT_EQ(v->GetTypeInfo(), "void");
  v = interpreter.FindVariableById("main$a");
  EXPECT_EQ(v->GetTypeInfo(), "list(int)");
  EXPECT_EQ(wamon::AsIntVariable(wamon::AsListVariable(v)->at(0))->GetValue(), 2);
  EXPECT_EQ(wamon::AsIntVariable(wamon::AsListVariable(v)->at(1))->GetValue(), 3);
  EXPECT_EQ(wamon::AsIntVariable(wamon::AsListVariable(v)->at(2))->GetValue(), 4);
  EXPECT_EQ(wamon::AsIntVariable(wamon::AsListVariable(v)->at(3))->GetValue(), 5);
}