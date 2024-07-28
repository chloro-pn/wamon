#include <iostream>
#include <string>

#include "wamon/interpreter.h"
#include "wamon/parser.h"
#include "wamon/scanner.h"
#include "wamon/type_checker.h"
#include "wamon/variable.h"

/*
 * lambda format : lambda [id_list] (param_list) -> type StatementBlock
 */

int main() {
  std::string script = R"(
    package main;

    let v0 : int = 1;

    let v : f((int, int) -> int) = lambda [v0] (int a, int b) -> int { return a + b + v0; };
  )";

  using namespace wamon;
  Scanner scan;
  auto tokens = scan.Scan(script);
  auto pu = Parse(tokens);
  pu = MergePackageUnits(std::move(pu));

  TypeChecker tc(pu);
  std::string reason;
  bool succ = tc.CheckAll(reason);
  if (succ == false) {
    std::cerr << reason << std::endl;
    return -1;
  }
  Interpreter ip(pu);
  std::vector<std::shared_ptr<Variable>> params;
  params.push_back(VariableFactory(TypeFactory<int>::Get(), Variable::ValueCategory::RValue, ip));
  params.push_back(VariableFactory(TypeFactory<int>::Get(), Variable::ValueCategory::RValue, ip));
  AsIntVariable(params[0])->SetValue(2);
  AsIntVariable(params[1])->SetValue(3);
  auto ret = ip.CallCallable(ip.FindVariableById("main$v"), std::move(params));
  std::cout << AsIntVariable(ret)->GetValue() << std::endl;
  return 0;
}
