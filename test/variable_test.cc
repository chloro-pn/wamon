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

  interpreter.CallFunctionByName("main$my_erase", {wamon::ToVar(0)});
  v = interpreter.FindVariableById("main$a");
  EXPECT_EQ(v->GetTypeInfo(), "list(int)");
  EXPECT_EQ(wamon::AsIntVariable(wamon::AsListVariable(v)->at(0))->GetValue(), 3);
  EXPECT_EQ(wamon::AsIntVariable(wamon::AsListVariable(v)->at(1))->GetValue(), 4);
  EXPECT_EQ(wamon::AsIntVariable(wamon::AsListVariable(v)->at(2))->GetValue(), 5);

  EXPECT_THROW(interpreter.CallFunctionByName("main$my_erase", {wamon::ToVar(3)}), wamon::WamonException);
}

TEST(variable, vo_var) {
  using namespace wamon;
  auto v = ToVar(2);
  EXPECT_EQ(v->GetTypeInfo(), "int");
  v = ToVar(2.5);
  EXPECT_EQ(v->GetTypeInfo(), "double");
  v = ToVar(true);
  EXPECT_EQ(v->GetTypeInfo(), "bool");
  v = ToVar((unsigned char)(12));
  EXPECT_EQ(v->GetTypeInfo(), "byte");
  v = ToVar(std::string("hello"));
  EXPECT_EQ(v->GetTypeInfo(), "string");
  EXPECT_EQ(AsStringVariable(v)->GetValue(), "hello");
}

wamon::PackageUnit GetPackageUnitFromScript(const std::string& script) {
  wamon::Scanner scan;
  auto tokens = scan.Scan(script);
  auto pu = wamon::Parse(tokens);
  pu = wamon::MergePackageUnits(std::move(pu));

  wamon::TypeChecker tc(pu);
  std::string reason;
  bool succ = tc.CheckAll(reason);
  if (succ == false) {
    throw wamon::WamonException("{}", reason);
  }
  return pu;
}

TEST(variable, print) {
  using namespace wamon;
  auto v = ToVar(24);
  EXPECT_EQ(v->Print().dump(), "24");
  v = ToVar((unsigned char)(33));
  EXPECT_EQ(v->Print().dump(), "\"0X21\"");
  v = ToVar(true);
  EXPECT_EQ(v->Print().dump(), "true");
  v = ToVar("hello world");
  EXPECT_EQ(v->Print().dump(), "\"hello world\"");
  v = ToVar(2.14);
  EXPECT_EQ(v->Print().dump(), "2.14");

  std::string script = R"(
    package main;

    let v : ptr(int) = alloc int(2);
    let v2 : f((int) -> int) = lambda [](int v) -> int { return v + 1; };

    struct ms {
      int a;
      double b;
      bool c;
    }

    let v3 : ms = (2, 2.3, true);
  )";
  auto pu = GetPackageUnitFromScript(script);
  wamon::Interpreter ip(pu);
  v = ip.FindVariableById("main$v");
  std::string print_result = R"({"point_to":2})";
  EXPECT_EQ(v->Print().dump(), print_result);
  v = ip.FindVariableById("main$v2");
  print_result = R"_({"function":"__lambda_0main","function_type":"f((int) -> int)"})_";
  EXPECT_EQ(v->Print().dump(), print_result);
  v = ip.FindVariableById("main$v3");
  print_result = R"({"members":{"a":2,"b":2.3,"c":true},"struct":"main$ms"})";
  EXPECT_EQ(v->Print().dump(), print_result);
}

TEST(variable, destructor) {
  std::string script = R"(
    package main;

    let count : int = 0;

    struct person {
      int age;
      string name;
    }

    method person {
      func getAge() -> int {
        return self.age;
      }

      destructor() {
        ++count;
      }
    }

    func test2() -> void {
      let v : list(person) = ();
      call v:resize(4);
    }

    func test3() -> int {
      let v : person = (2, "chloro");
      // 因为move v是右值，因此调用其方法会将成员变量move走，第二次调用的时候只能获得对应类型的默认值(int : 0)
      call move v:getAge();
      return call v:getAge();
    }
  )";

  auto pu = GetPackageUnitFromScript(script);
  wamon::TypeChecker tc(pu);
  wamon::Interpreter ip(pu);
  auto v = ip.ExecExpression(tc, "main", R"(new person(2, "chloro"))");
  v.reset();
  v = ip.FindVariableById("main$count");
  EXPECT_EQ(wamon::VarAs<int>(v), 1);
  ip.CallFunctionByName("main$test2", {});
  v = ip.FindVariableById("main$count");
  EXPECT_EQ(wamon::VarAs<int>(v), 5);
  v = ip.CallFunctionByName("main$test3", {});
  EXPECT_EQ(wamon::VarAs<int>(v), 0);
}
