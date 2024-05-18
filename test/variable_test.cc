#include "wamon/variable.h"

#include <vector>

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

    func my_erase(int i) -> void {
      call a:erase(i);
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
  EXPECT_EQ(succ, true) << reason;
  wamon::Interpreter interpreter(pu);
  auto v = interpreter.CallFunctionByName("main$my_func", {});
  EXPECT_EQ(v->GetTypeInfo(), "void");
  v = interpreter.FindVariableById("main$a");
  EXPECT_EQ(v->GetTypeInfo(), "list(int)");
  EXPECT_EQ(wamon::AsIntVariable(wamon::AsListVariable(v)->at(0))->GetValue(), 2);
  EXPECT_EQ(wamon::AsIntVariable(wamon::AsListVariable(v)->at(1))->GetValue(), 3);
  EXPECT_EQ(wamon::AsIntVariable(wamon::AsListVariable(v)->at(2))->GetValue(), 4);
  EXPECT_EQ(wamon::AsIntVariable(wamon::AsListVariable(v)->at(3))->GetValue(), 5);

  std::vector<std::shared_ptr<wamon::Variable>> params;
  params.push_back(
      wamon::VariableFactoryShared(wamon::TypeFactory<int>::Get(), wamon::Variable::ValueCategory::RValue, "", pu));
  wamon::AsIntVariable(params[0])->SetValue(0);
  interpreter.CallFunctionByName("main$my_erase", std::move(params));
  v = interpreter.FindVariableById("main$a");
  EXPECT_EQ(v->GetTypeInfo(), "list(int)");
  EXPECT_EQ(wamon::AsIntVariable(wamon::AsListVariable(v)->at(0))->GetValue(), 3);
  EXPECT_EQ(wamon::AsIntVariable(wamon::AsListVariable(v)->at(1))->GetValue(), 4);
  EXPECT_EQ(wamon::AsIntVariable(wamon::AsListVariable(v)->at(2))->GetValue(), 5);

  params.clear();
  params.push_back(
      wamon::VariableFactoryShared(wamon::TypeFactory<int>::Get(), wamon::Variable::ValueCategory::RValue, "", pu));
  wamon::AsIntVariable(params[0])->SetValue(3);
  EXPECT_THROW(interpreter.CallFunctionByName("main$my_erase", std::move(params)), wamon::WamonExecption);
}
