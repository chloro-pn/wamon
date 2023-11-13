#include "wamon/scanner.h"
#include "wamon/parser.h"
#include "wamon/interpreter.h"
#include "wamon/variable.h"

#include "gtest/gtest.h"

TEST(interpreter, callfunction) {
  wamon::Scanner scan;
  std::string str = R"(
    package main;

    let a : int = (2);
    let b : string = ("nanpang");

    func my_func() -> int {
      return a;
    }
  )";
  wamon::PackageUnit pu;
  auto tokens = scan.Scan(str);
  pu = wamon::Parse(tokens);
  wamon::Interpreter interpreter(pu);
  auto v = interpreter.CallFunctionByName("my_func", {});
  EXPECT_EQ(v->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(v)->GetValue(), 2);
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
  )";
  wamon::PackageUnit pu;
  auto tokens = scan.Scan(str);
  pu = wamon::Parse(tokens);
  wamon::Interpreter interpreter(pu);
  auto v = interpreter.FindVariableById("mydata");
  EXPECT_EQ(v->GetTypeInfo(), "my_struct_name");
  auto v2 = dynamic_cast<wamon::StructVariable*>(v.get());
  EXPECT_EQ(wamon::AsIntVariable(v2->GetDataMemberByName("a"))->GetValue(), 2);
}