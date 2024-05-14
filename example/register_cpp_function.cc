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
      return call wamon::my_cpp_func:(s);
    }
  )";

  wamon::Scanner scanner;
  auto tokens = scanner.Scan(script);
  wamon::PackageUnit package_unit = wamon::Parse(tokens);
  package_unit = wamon::MergePackageUnits(std::move(package_unit));

  wamon::Interpreter ip(package_unit);

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

  wamon::TypeChecker type_checker(package_unit);

  std::string reason;
  bool succ = type_checker.CheckAll(reason);
  if (succ == false) {
    std::cerr << "type check error : " << reason << std::endl;
    return -1;
  }

  auto string_v = wamon::VariableFactory(wamon::TypeFactory<std::string>::Get(), wamon::Variable::ValueCategory::RValue,
                                         "", ip.GetPackageUnit());
  wamon::AsStringVariable(string_v)->SetValue("hello");

  auto ret = ip.CallFunctionByName("main$call_cpp_function", {std::move(string_v)});
  std::cout << wamon::AsIntVariable(ret)->GetValue() << std::endl;
  return 0;
}