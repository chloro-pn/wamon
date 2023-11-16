#include "wamon/scanner.h"
#include "wamon/parser.h"
#include "wamon/interpreter.h"
#include "wamon/variable.h"
#include "wamon/static_analyzer.h"
#include "wamon/type_checker.h"
#include "gtest/gtest.h"

TEST(interpreter, callfunction) {
  wamon::Scanner scan;
  std::string str = R"(
    package main;

    let a : int = (2);
    let b : string = ("nanpang");

    func my_func() -> int {
      return a + 2;
    }
  )";
  wamon::PackageUnit pu;
  auto tokens = scan.Scan(str);
  pu = wamon::Parse(tokens);
  wamon::Interpreter interpreter(pu);
  auto v = interpreter.CallFunctionByName("my_func", {});
  EXPECT_EQ(v->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(v)->GetValue(), 4);
}

TEST(interpreter, variable) {
  wamon::Scanner scan;
  std::string str = R"(
    package main;

    struct my_struct_name {
      int a;
      double b;
      string c;
    }

    let mydata : my_struct_name = (2, 3.5, "hello");
    let myptr : ptr(my_struct_name) = (&mydata);
    let mylen : int = (call len("hello"));
    let mylist : list(int) = (2, 3, 4);
  )";
  wamon::PackageUnit pu;
  auto tokens = scan.Scan(str);
  pu = wamon::Parse(tokens);

  wamon::StaticAnalyzer sa(pu);
  wamon::TypeChecker tc(sa);
  tc.CheckTypes();
  tc.CheckAndRegisterGlobalVariable();
  tc.CheckStructs();
  tc.CheckFunctions();
  tc.CheckMethods();
  //tc.CheckOperatorOverride();

  wamon::Interpreter interpreter(pu);
  auto v = interpreter.FindVariableById("mydata");
  EXPECT_EQ(v->GetTypeInfo(), "my_struct_name");
  auto v2 = dynamic_cast<wamon::StructVariable*>(v.get());
  EXPECT_EQ(wamon::AsIntVariable(v2->GetDataMemberByName("a"))->GetValue(), 2);

  auto old_v = v;
  v = interpreter.FindVariableById("myptr");
  EXPECT_EQ(v->GetTypeInfo(), "ptr(my_struct_name)");
  auto v3 = dynamic_cast<wamon::PointerVariable*>(v.get());
  EXPECT_EQ(v3->GetHoldVariable(), old_v);

  v = interpreter.FindVariableById("mylen");
  EXPECT_EQ(v->GetTypeInfo(), "int");
  auto v4 = dynamic_cast<wamon::IntVariable*>(v.get());
  EXPECT_EQ(v4->GetValue(), 5);

  v = interpreter.FindVariableById("mylist");
  EXPECT_EQ(v->GetTypeInfo(), "list(int)");
  auto v5 = dynamic_cast<wamon::ListVariable*>(v.get());
  EXPECT_EQ(v5->Size(), 3);
  EXPECT_EQ(wamon::AsIntVariable(v5->at(0))->GetValue(), 2);
}

TEST(interpreter, callmethod) {
  wamon::Scanner scan;
  std::string str = R"(
    package main;

    struct my_struct_name {
      int a;
      double b;
      string c;
    }

    method my_struct_name {
      func get_age() -> int {
        return self.a;
      }
    }

    let ms : my_struct_name = (25, 2.1, "bob");
  )";
  wamon::PackageUnit pu;
  auto tokens = scan.Scan(str);
  pu = wamon::Parse(tokens);

  wamon::StaticAnalyzer sa(pu);
  wamon::TypeChecker tc(sa);
  tc.CheckTypes();
  tc.CheckAndRegisterGlobalVariable();
  tc.CheckStructs();
  tc.CheckFunctions();
  tc.CheckMethods();

  wamon::Interpreter interpreter(pu);
  auto v = interpreter.FindVariableById("ms");
  auto ret = interpreter.CallMethodByName(v, "get_age", {});
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 25);
}

TEST(interpreter, operator) {
  wamon::Scanner scan;
  std::string str = R"(
    package main;

    func stradd(string a, string b) -> string {
      return a + b;
    }
  )";
  wamon::PackageUnit pu;
  auto tokens = scan.Scan(str);
  pu = wamon::Parse(tokens);

  wamon::StaticAnalyzer sa(pu);
  wamon::TypeChecker tc(sa);
  tc.CheckTypes();
  tc.CheckAndRegisterGlobalVariable();
  tc.CheckStructs();
  tc.CheckFunctions();
  tc.CheckMethods();

  wamon::Interpreter interpreter(pu);
  std::vector<std::shared_ptr<wamon::Variable>> params;
  params.push_back(std::shared_ptr<wamon::Variable>(new wamon::StringVariable("hello ", "")));
  params.push_back(std::shared_ptr<wamon::Variable>(new wamon::StringVariable("world", "")));
  auto ret = interpreter.CallFunctionByName("stradd", std::move(params));
  EXPECT_EQ(ret->GetTypeInfo(), "string");
  EXPECT_EQ(wamon::AsStringVariable(ret)->GetValue(), "hello world");
}