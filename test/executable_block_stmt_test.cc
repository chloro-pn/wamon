#include "wamon/executable_block_stmt.h"

#include <wamon/ast.h>
#include <wamon/package_unit.h>
#include <wamon/type_checker.h>
#include <wamon/variable_int.h>
#include <wamon/variable_string.h>

#include <memory>
#include <string>

#include "gtest/gtest.h"
#include "wamon/function_def.h"
#include "wamon/interpreter.h"
#include "wamon/parser.h"
#include "wamon/scanner.h"
#include "wamon/type.h"
#include "wamon/variable.h"

using namespace wamon;

std::unique_ptr<ExecutableBlockStmt> get_executable_block_stmt() {
  auto executable_block_stmt = [](Interpreter& ip) -> std::shared_ptr<Variable> {
    auto va = ip.FindVariableById("main$param_a");
    auto vref = ip.FindVariableById("main$param_ref");
    va = ip.CallFunctionByName("main$update_value_from_script", {va});
    int ret = AsIntVariable(va)->GetValue();
    AsStringVariable(vref)->SetValue("changed");
    return std::make_shared<IntVariable>(ret * 2, Variable::ValueCategory::RValue, "");
  };
  auto executable_block = std::make_unique<ExecutableBlockStmt>();
  executable_block->SetExecutable(std::move(executable_block_stmt));
  return executable_block;
}

std::shared_ptr<FunctionDef> generate_func_for_test() {
  std::shared_ptr<FunctionDef> fd(new FunctionDef("my_fn"));
  fd->AddParamList({
      .name = "main$param_a",
      .type = TypeFactory<int>::Get(),
      .is_ref = false,
  });

  fd->AddParamList({
      .name = "main$param_ref",
      .type = TypeFactory<std::string>::Get(),
      .is_ref = true,
  });

  fd->SetReturnType(TypeFactory<int>::Get());
  fd->SetBlockStmt(get_executable_block_stmt());
  return fd;
}

TEST(executable_block_stmt, basic) {
  std::string script = R"(
    package main;

    func update_value_from_script(int a) -> int {
      return a + 10;
    }
  )";

  Scanner scan;
  auto tokens = scan.Scan(script);
  auto pu = Parse(tokens);
  pu.AddFuncDef(generate_func_for_test());
  pu = MergePackageUnits(std::move(pu));
  auto va = std::make_shared<IntVariable>(1, Variable::ValueCategory::RValue, "");
  auto vb = std::make_shared<StringVariable>("hello", Variable::ValueCategory::LValue, "");
  TypeChecker tc(pu);
  std::string reason;
  bool succ = tc.CheckAll(reason);
  EXPECT_EQ(succ, true) << reason;

  Interpreter ip(pu);
  auto ret = ip.CallFunctionByName("main$my_fn", {va, vb});
  EXPECT_EQ(AsIntVariable((ret))->GetValue(), 22);
  EXPECT_EQ(AsStringVariable(vb)->GetValue(), "changed");
}
