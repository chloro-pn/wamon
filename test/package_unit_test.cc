#include "gtest/gtest.h"
// just for test
#define private public
#include "wamon/interpreter.h"
#include "wamon/package_unit.h"
#include "wamon/parser.h"
#include "wamon/scanner.h"
#include "wamon/type_checker.h"

TEST(package_unit, basic) {
  wamon::Scanner scan;

  std::string str = R"(
    package main;

    import net;
    import os;

    struct my_struct_name {
      int a;
      double b;
      string c;
    }

    func my_func(int age, string name) -> void {}

    let my_var : bool = (call init:());

    method my_struct_name {
      func get_a() -> int {
        return self.a;
      }

      func get_b() -> double {
        return self.b;
      }
    }
  )";
  wamon::PackageUnit pu;
  auto tokens = scan.Scan(str);
  pu = wamon::Parse(tokens);
  EXPECT_EQ(pu.package_name_, "main");
  EXPECT_EQ(pu.import_packages_.size(), 2);
  EXPECT_EQ(pu.funcs_.size(), 1);
  EXPECT_EQ(pu.structs_.size(), 1);
  EXPECT_EQ(pu.var_define_.size(), 1);
  bool find = false;
  find = pu.funcs_.count("my_func") > 0;
  EXPECT_EQ(find, true);
  find = pu.structs_.count("my_struct_name") > 0;
  EXPECT_EQ(find, true);
  const auto& tmp = pu.var_define_[0]->GetType();
  EXPECT_EQ(wamon::IsBoolType(tmp), true);
}

TEST(package_unit, multi_package) {
  wamon::Scanner scan;

  std::string str = R"(
    package main;

    import math;

    func calculate() -> int {
      return call math::add:(1);
    }
  )";

  std::string math_str = R"(
    package math;

    func add(int i) -> int {
      return i + 10;
    }
  )";

  std::string math_str2 = R"(
    package math;

    let v : int = 10;
  )";
  wamon::PackageUnit pu;
  wamon::PackageUnit math_pu;
  wamon::PackageUnit math2_pu;
  auto tokens = scan.Scan(str);
  pu = wamon::Parse(tokens);

  tokens = scan.Scan(math_str);
  math_pu = wamon::Parse(tokens);

  tokens = scan.Scan(math_str2);
  math2_pu = wamon::Parse(tokens);

  pu = wamon::MergePackageUnits(std::move(pu), std::move(math_pu), std::move(math2_pu));

  wamon::TypeChecker tc(pu);
  std::string reason;
  bool succ = tc.CheckAll(reason);
  EXPECT_EQ(succ, true) << reason;
  wamon::Interpreter ip(pu);
  auto v = ip.CallFunctionByName("main$calculate", {});
  EXPECT_EQ(v->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(v)->GetValue(), 11);
  v = ip.FindVariableById("math$v");
  EXPECT_EQ(wamon::AsIntVariable(v)->GetValue(), 10);
}

TEST(package_unit, import_check) {
  wamon::Scanner scan;
  std::string main_script = R"(
    package main;

    let v : int = net::v;
  )";

  auto tokens = scan.Scan(main_script);
  EXPECT_THROW(wamon::Parse(tokens), wamon::WamonExecption);
}
