#include <iostream>
#include <string>

#include "wamon/interpreter.h"
#include "wamon/parser.h"
#include "wamon/scanner.h"
#include "wamon/type_checker.h"
#include "wamon/variable.h"

int main() {
  std::string script = R"(
    package main;

    func call_cpp_function(string s) -> int {
      return call my_cpp_func:(s);
    }

    let test : f(() -> int) = my_type_cpp_func;
  )";

  wamon::Scanner scanner;
  auto tokens = scanner.Scan(script);
  wamon::PackageUnit package_unit = wamon::Parse(tokens);
  package_unit = wamon::MergePackageUnits(std::move(package_unit));

  // 这里有问题，构造函数中定义了全局变量，然后才注册函数，这是有问题的，fix：将全局变量的定义独立出来不放在构造函数中
  wamon::Interpreter ip(package_unit, wamon::Interpreter::Tag::DelayConstruct);

  ip.RegisterCppFunctions(
      "my_cpp_func",
      [](const std::vector<std::unique_ptr<wamon::Type>>& params_type) -> std::unique_ptr<wamon::Type> {
        if (params_type.size() != 1) {
          throw wamon::WamonExecption("invalid params count {}", params_type.size());
        }
        if (!wamon::IsStringType(params_type[0])) {
          throw wamon::WamonExecption("invalid params type {}", params_type[0]->GetTypeInfo());
        }
        return wamon::TypeFactory<int>::Get();
      },
      [](std::vector<std::shared_ptr<wamon::Variable>>&& params) -> std::shared_ptr<wamon::Variable> {
        auto len = wamon::AsStringVariable(params[0])->GetValue().size();
        return std::make_shared<wamon::IntVariable>(static_cast<int>(len), wamon::Variable::ValueCategory::RValue, "");
      });

  ip.RegisterCppFunctions(
      "my_type_cpp_func", wamon::TypeFactory<int()>::Get(),
      [](std::vector<std::shared_ptr<wamon::Variable>>&& params) -> std::shared_ptr<wamon::Variable> {
        return std::make_shared<wamon::IntVariable>(12138, wamon::Variable::ValueCategory::RValue, "");
      });

  wamon::TypeChecker type_checker(package_unit);

  std::string reason;
  bool succ = type_checker.CheckAll(reason);
  if (succ == false) {
    std::cerr << "type check error : " << reason << std::endl;
    return -1;
  }
  ip.ExecGlobalVariDefStmt();
  auto string_v = wamon::VariableFactory(wamon::TypeFactory<std::string>::Get(), wamon::Variable::ValueCategory::RValue,
                                         "", ip.GetPackageUnit());
  wamon::AsStringVariable(string_v)->SetValue("hello");

  auto ret = ip.CallFunctionByName("main$call_cpp_function", {std::move(string_v)});
  std::cout << wamon::AsIntVariable(ret)->GetValue() << std::endl;

  ret = ip.CallCallable(ip.FindVariableById("main$test"), {});
  std::cout << wamon::AsIntVariable(ret)->GetValue() << std::endl;
  return 0;
}
