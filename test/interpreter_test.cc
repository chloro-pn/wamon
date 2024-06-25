#include "wamon/interpreter.h"

#include "gtest/gtest.h"
#include "wamon/parser.h"
#include "wamon/scanner.h"
#include "wamon/static_analyzer.h"
#include "wamon/type_checker.h"
#include "wamon/variable.h"

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

  pu = wamon::MergePackageUnits(std::move(pu));

  wamon::TypeChecker tc(pu);
  std::string reason;
  bool succ = tc.CheckAll(reason);
  EXPECT_EQ(succ, true) << reason;
  wamon::Interpreter interpreter(pu);
  auto v = interpreter.CallFunctionByName("main$my_func", {});
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

    func add(int a, int b) -> int {
      return a + b;
    }

    let mydata : my_struct_name = (2, 3.5, "hello");
    let mystr : string = ("hello");
    let myptr : ptr(my_struct_name) = (&mydata);
    let mylen : int = (call mystr:len());
    let mylist : list(int) = (2, 3, 4);
    let myfunc : f((int, int) -> int) = (add);

    let myfunc2 : f((int, int) -> int) = (myfunc);
    let call_ret : int = (call myfunc2:(2, 3));

    func update_list() -> int {
      mylist[1] = mylist[1] * 2;
      return mylist[1];
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
  // tc.CheckOperatorOverride();

  wamon::Interpreter interpreter(pu);

  auto v = interpreter.FindVariableById("main$mydata");
  EXPECT_EQ(v->GetTypeInfo(), "main$my_struct_name");
  auto v2 = dynamic_cast<wamon::StructVariable*>(v.get());
  EXPECT_EQ(wamon::AsIntVariable(v2->GetDataMemberByName("a"))->GetValue(), 2);
  auto old_v = v;
  v = interpreter.FindVariableById("main$myptr");
  EXPECT_EQ(v->GetTypeInfo(), "ptr(main$my_struct_name)");
  auto v3 = dynamic_cast<wamon::PointerVariable*>(v.get());
  EXPECT_EQ(v3->GetHoldVariable(), old_v);

  v = interpreter.FindVariableById("main$mylen");
  EXPECT_EQ(v->GetTypeInfo(), "int");
  auto v4 = dynamic_cast<wamon::IntVariable*>(v.get());
  EXPECT_EQ(v4->GetValue(), 5);

  v = interpreter.FindVariableById("main$mylist");
  EXPECT_EQ(v->GetTypeInfo(), "list(int)");
  auto v5 = dynamic_cast<wamon::ListVariable*>(v.get());
  EXPECT_EQ(v5->Size(), 3);
  EXPECT_EQ(wamon::AsIntVariable(v5->at(0))->GetValue(), 2);

  v = interpreter.FindVariableById("main$myfunc");
  EXPECT_EQ(v->GetTypeInfo(), "f((int, int) -> int)");
  v = interpreter.FindVariableById("main$myfunc2");
  EXPECT_EQ(v->GetTypeInfo(), "f((int, int) -> int)");

  v = interpreter.FindVariableById("main$call_ret");
  EXPECT_EQ(v->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(v)->GetValue(), 5);

  auto ret = interpreter.CallFunctionByName("main$update_list", {});
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 6);
}

TEST(interpreter, variable_compare) {
  wamon::Scanner scan;
  std::string str = R"(
    package main;

    struct mstruct {
      int a;
      bool b;
    }

    let a : int = (2);
    let b : string = ("bob");
    let c : int = (3);
    let d : string = ("bob");
    let e : mstruct = (a, true);
    let g : mstruct = (c, true);
    let h : list(int) = (2, 3, 4);
    let i : list(int) = (2, 3, 4);
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
  auto v = interpreter.FindVariableById("main$a");
  auto v2 = interpreter.FindVariableById("main$c");
  EXPECT_EQ(v->Compare(v2), false);

  v = interpreter.FindVariableById("main$b");
  v2 = interpreter.FindVariableById("main$d");
  EXPECT_EQ(v->Compare(v2), true);

  v = interpreter.FindVariableById("main$e");
  v2 = interpreter.FindVariableById("main$g");
  EXPECT_EQ(v->Compare(v2), false);

  v = interpreter.FindVariableById("main$h");
  v2 = interpreter.FindVariableById("main$i");
  EXPECT_EQ(v->Compare(v2), true);
}

TEST(interpreter, variable_assign) {
  wamon::Scanner scan;
  std::string str = R"(
    package main;

    struct mstruct {
      int a;
      bool b;
    }

    let a : int = (2);
    let b : string = ("bob");
    let c : int = (3);
    let d : string = ("bob");
    let e : mstruct = (a, true);
    let g : mstruct = (c, true);
    let h : list(int) = (2, 3, 4);
    let i : list(int) = (2, 3, 4);
    let j : list(int) = (0);
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
  auto v = interpreter.FindVariableById("main$a");
  auto v2 = interpreter.FindVariableById("main$c");
  v->Assign(v2);
  EXPECT_EQ(wamon::AsIntVariable(v)->GetValue(), 3);

  v = interpreter.FindVariableById("main$b");
  v2 = interpreter.FindVariableById("main$d");
  v->Assign(v2);
  EXPECT_EQ(wamon::AsStringVariable(v)->GetValue(), "bob");

  v = interpreter.FindVariableById("main$e");
  v2 = interpreter.FindVariableById("main$g");
  v->Assign(v2);
  EXPECT_EQ(wamon::AsBoolVariable(wamon::AsStructVariable(v)->GetDataMemberByName("b"))->GetValue(), true);

  v = interpreter.FindVariableById("main$h");
  v2 = interpreter.FindVariableById("main$j");
  auto list_v2 = wamon::AsListVariable(v2);
  v->Assign(v2);
  EXPECT_EQ(list_v2->Size(), 1);
  EXPECT_EQ(wamon::AsIntVariable(list_v2->at(0))->GetValue(), 0);
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
    let clen : int = (call ms.c:len());
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
  auto v = interpreter.FindVariableById("main$ms");
  auto ret = interpreter.CallMethodByName(v, "get_age", {});
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 25);

  v = interpreter.FindVariableById("main$clen");
  EXPECT_EQ(v->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(v)->GetValue(), 3);
}

TEST(interpreter, inner_type_method) {
  wamon::Scanner scan;
  std::string str = R"(
    package main;

    let v1 : string = ("hello");
    let v2 : list(int) = (2, 3, 4);

    func func1() -> int {
      return call v1:len();
    }

    func func2() -> byte {
      return call v1:at(0);
    }

    func func3() -> int {
      return call v2:size() + call v2:at(1);
      // 6
    }

    func func4() -> string {
       call v1:append(" world");
       call v1:append(new byte(0X21));
      return v1;
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
  auto ret = interpreter.CallFunctionByName("main$func1", {});
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 5);

  ret = interpreter.CallFunctionByName("main$func2", {});
  EXPECT_EQ(ret->GetTypeInfo(), "byte");
  EXPECT_EQ(wamon::AsByteVariable(ret)->GetValue(), 'h');

  ret = interpreter.CallFunctionByName("main$func3", {});
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 6);

  ret = interpreter.CallFunctionByName("main$func4", {});
  EXPECT_EQ(wamon::AsStringVariable(ret)->GetValue(), "hello world!");
}

TEST(interpreter, builtin_function) {
  wamon::Scanner scan;
  std::string str = R"(
    package main;
    let v : string = call wamon::to_string:(20);
    let v2 : string = call wamon::to_string:(true);
    let v3 : string = call wamon::to_string:(3.35);
    let v4 : string = call wamon::to_string:(0X41);

    let v5 : string = @wamon::to_string:(14);
  )";
  wamon::PackageUnit pu;
  auto tokens = scan.Scan(str);
  pu = wamon::Parse(tokens);
  pu = wamon::MergePackageUnits(std::move(pu));
  wamon::TypeChecker tc(pu);
  std::string reason;
  bool succ = tc.CheckAll(reason);
  EXPECT_EQ(succ, true) << reason;

  wamon::Interpreter ip(pu);
  auto v = ip.FindVariableById("main$v");
  auto v2 = ip.FindVariableById("main$v2");
  auto v3 = ip.FindVariableById("main$v3");
  auto v4 = ip.FindVariableById("main$v4");
  auto v5 = ip.FindVariableById("main$v5");
  EXPECT_EQ(wamon::AsStringVariable(v)->GetValue(), "20");
  EXPECT_EQ(wamon::AsStringVariable(v2)->GetValue(), "true");
  EXPECT_EQ(wamon::AsStringVariable(v3)->GetValue(), "3.350000");
  EXPECT_EQ(wamon::AsStringVariable(v4)->GetValue(), "A");
  EXPECT_EQ(wamon::AsStringVariable(v5)->GetValue(), "14");
}

TEST(interpreter, callable) {
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

      operator () (int a) -> int {
        return self.a + a;
      }
    }

    let v : int = (2);
    let ms : my_struct_name = (25, 2.1, "bob");
    let mycallable : f((int) -> int) = (ms);

    func test_func(int a) -> int {
      return a + 1;
    }

    func call_test() -> int {
      // callable对象，持有原始函数
      let f1 : f((int) -> int) = (test_func);
      // callable对象，持有运算符重载的结构体对象
      let f2 : f((int) -> int) = (ms);
      // callable对象，通过其他callable对象构造
      let f3 : f((int) -> int) = (f2);

      return call f1:(0) + call f2:(0) + call ms:(-20) + call f3:(0);
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
  auto v = interpreter.FindVariableById("main$mycallable");
  auto a = interpreter.FindVariableById("main$v");

  auto ret = interpreter.CallCallable(v, {a});
  EXPECT_EQ(ret->GetTypeInfo(), "int");

  ret = interpreter.CallFunctionByName("main$call_test", {});
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 56);
}

TEST(interpreter, trait) {
  wamon::Scanner scan;
  std::string str = R"(
    package main;

    struct s1 {
      int a;
      double b;
    }

    method s1 {
      func get_age() -> int {
        return self.a + 10;
      }
    }

    struct s2 {
      int a;
      string c;
    }

    method s2 {
      func get_age() -> int {
        return self.a;
      }

      func get_name() -> string {
        return self.c;
      }
    }

    struct trait have_age {
      int a;
    }

    method have_age {
      func get_age() -> int;
    }

    struct trait have_age_and_name {
      int a;
    }

    method have_age_and_name {
      func get_age() -> int;
      func get_name() -> string;
    }

    let t1 : s1 = (0, 3.4);
    let t2 : s2 = (0, "bob");

    let v0 : have_age_and_name = move t2;

    let v1 : have_age = move t1;
    let v2 : have_age = move v0;

    func test() -> bool {
      return v1 == v2;
    }

    func call_trait_get_age(have_age v) -> int {
      return call v:get_age();
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
  auto v = interpreter.FindVariableById("main$v1");
  EXPECT_EQ(wamon::AsStructVariable(v)->GetDataMemberByName("a")->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsStructVariable(v)->GetStructDef()->GetStructName(), "main$s1");
  auto ret = interpreter.CallFunctionByName("main$test", {});
  EXPECT_EQ(ret->GetTypeInfo(), "bool");
  EXPECT_EQ(wamon::AsBoolVariable(ret)->GetValue(), true);

  ret = interpreter.CallFunctionByName("main$call_trait_get_age", {v});
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 10);
}

TEST(interpreter, operator) {
  wamon::Scanner scan;
  std::string str = R"(
    package main;

    func stradd(string a, string b) -> string {
      return a + b;
    }

    func intminus(int a, int b) -> int {
      return a - b;
    }

    func intmulti(int a, int b) -> int {
      return a * b;
    }

    func intdivide(int a, int b) -> int {
      return a / b;
    }

    func intuoperator(int a) -> int {
      return -a;
    }

    func boolnot(bool b) -> bool {
      return !b;
    }

    operator | (int a, int update) -> int {
      return a + update;
    }
  
    func op_override() -> int {
      return 2 | 3;
    }

    struct myint {
      int a;
    }

    let v1 : myint = 2;
    let v2 : myint = 3;

    operator + (myint a, myint b) -> myint {
      let tmp : myint = (a.a + b.a);
      return tmp;
    }

    func op_override2() -> int {
      return (v1 + v2).a;
    }

    func as_test(int a) -> double {
      return a as double;
    }

    struct s1 {
      int a;
      string b;
    }

    struct trait st {
      int a;
    }

    let s1v : s1 = (2, "bob");
    
    func as_test_2() -> st {
      return s1v as st;
    }

    func as_test_3() -> string {
      let bytes : list(byte) = (0X61, 0X62, 0X63);
      return bytes as string;
    }

    func as_test_4() -> int {
      let v : double = 3.9;
      return v as int;
    }

    enum Color {
      Red;
      Blue;
    }

    func as_test_5() -> string {
      let v : Color = enum Color:Blue;
      return v as string;
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
  auto ret = interpreter.CallFunctionByName("main$stradd", {wamon::ToVar("hello"), wamon::ToVar(" world")});
  EXPECT_EQ(ret->GetTypeInfo(), "string");
  EXPECT_EQ(wamon::AsStringVariable(ret)->GetValue(), "hello world");

  ret = interpreter.CallFunctionByName("main$intminus", {wamon::ToVar(10), wamon::ToVar(5)});
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 5);

  ret = interpreter.CallFunctionByName("main$intmulti", {wamon::ToVar(10), wamon::ToVar(5)});
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 50);

  ret = interpreter.CallFunctionByName("main$intdivide", {wamon::ToVar(10), wamon::ToVar(5)});
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 2);

  ret = interpreter.CallFunctionByName("main$intuoperator", {wamon::ToVar(10)});
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), -10);

  ret = interpreter.CallFunctionByName("main$boolnot", {wamon::ToVar(true)});
  EXPECT_EQ(ret->GetTypeInfo(), "bool");
  EXPECT_EQ(wamon::AsBoolVariable(ret)->GetValue(), false);

  ret = interpreter.CallFunctionByName("main$op_override", {});
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 5);

  auto v1 = interpreter.FindVariableById("main$v1");
  auto v2 = interpreter.FindVariableById("main$v2");
  ret = interpreter.CallFunctionByName("main$op_override2", {});
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 5);

  auto int_v =
      wamon::VariableFactory(wamon::TypeFactory<int>::Get(), wamon::Variable::ValueCategory::RValue, "", interpreter);
  wamon::AsIntVariable(int_v)->SetValue(2);
  ret = interpreter.CallFunctionByName("main$as_test", {int_v});
  EXPECT_EQ(ret->GetTypeInfo(), "double");
  EXPECT_DOUBLE_EQ(wamon::AsDoubleVariable(ret)->GetValue(), 2.0);

  ret = interpreter.CallFunctionByName("main$as_test_2", {});
  EXPECT_EQ(ret->GetTypeInfo(), "main$st");
  EXPECT_DOUBLE_EQ(wamon::AsIntVariable(wamon::AsStructVariable(ret)->GetDataMemberByName("a"))->GetValue(), 2);

  ret = interpreter.CallFunctionByName("main$as_test_3", {});
  EXPECT_EQ(ret->GetTypeInfo(), "string");
  EXPECT_EQ(wamon::AsStringVariable(ret)->GetValue(), "abc");

  ret = interpreter.CallFunctionByName("main$as_test_4", {});
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 3);

  ret = interpreter.CallFunctionByName("main$as_test_5", {});
  EXPECT_EQ(ret->GetTypeInfo(), "string");
  EXPECT_EQ(wamon::AsStringVariable(ret)->GetValue(), "main$Color:Blue");

  ret = interpreter.ExecExpression(tc, "main", "0X30 as int");
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 48);

  ret = interpreter.ExecExpression(tc, "main", "48 as byte");
  EXPECT_EQ(ret->GetTypeInfo(), "byte");
  EXPECT_EQ((char)wamon::AsByteVariable(ret)->GetValue(), '0');

  ret = interpreter.ExecExpression(tc, "main", "++2");
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 3);

  ret = interpreter.ExecExpression(tc, "main", "2 < 3");
  EXPECT_EQ(wamon::VarAs<bool>(ret), true);

  ret = interpreter.ExecExpression(tc, "main", "2 <= 3");
  EXPECT_EQ(wamon::VarAs<bool>(ret), true);

  ret = interpreter.ExecExpression(tc, "main", "3 < 2");
  EXPECT_EQ(wamon::VarAs<bool>(ret), false);

  ret = interpreter.ExecExpression(tc, "main", "3 <= 3");
  EXPECT_EQ(wamon::VarAs<bool>(ret), true);

  ret = interpreter.ExecExpression(tc, "main", "2 > 3");
  EXPECT_EQ(wamon::VarAs<bool>(ret), false);

  ret = interpreter.ExecExpression(tc, "main", "3 >= 3");
  EXPECT_EQ(wamon::VarAs<bool>(ret), true);
}

TEST(interpreter, fibonacci) {
  wamon::Scanner scan;
  std::string str = R"(
    package main;

    func Fibonacci(int n) -> int {
      if (n == 0) {
        return 0;
      }
      if (n == 1) {
        return 1;
      }
      return call Fibonacci:(n - 1) + call Fibonacci:(n - 2);
    }

    let v : int = (10);
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
  auto a = interpreter.FindVariableById("main$v");
  auto ret = interpreter.CallFunctionByName("main$Fibonacci", {a});
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 55);
}

TEST(interpreter, register_cpp_function) {
  wamon::Scanner scan;
  std::string str = R"(
    package main;

    func testfunc() -> int {
      let v2 : int = call func111:("hello");
      return v2;
    }
  )";
  wamon::PackageUnit pu;
  auto tokens = scan.Scan(str);
  pu = wamon::Parse(tokens);

  pu.RegisterCppFunctions(
      "func111",
      [](const wamon::PackageUnit&,
         const std::vector<std::unique_ptr<wamon::Type>>& params_type) -> std::unique_ptr<wamon::Type> {
        if (params_type.size() != 1) {
          throw wamon::WamonException("invalid params count {}", params_type.size());
        }
        if (!wamon::IsStringType(params_type[0])) {
          throw wamon::WamonException("invalid params type {}", params_type[0]->GetTypeInfo());
        }
        return wamon::TypeFactory<int>::Get();
      },
      [](wamon::Interpreter&,
         std::vector<std::shared_ptr<wamon::Variable>>&& params) -> std::shared_ptr<wamon::Variable> {
        auto len = wamon::AsStringVariable(params[0])->GetValue().size();
        return std::make_shared<wamon::IntVariable>(static_cast<int>(len), wamon::Variable::ValueCategory::RValue, "");
      });

  pu = wamon::MergePackageUnits(std::move(pu));
  wamon::TypeChecker tc(pu);
  std::string reason;
  bool succ = tc.CheckAll(reason);
  EXPECT_EQ(succ, true) << reason;

  wamon::Interpreter interpreter(pu);
  auto ret = interpreter.CallFunctionByName("main$testfunc", {});
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 5);
}

TEST(interpreter, move) {
  wamon::Scanner scan;
  std::string str = R"(
    package main;

    let a : int = (2);
    let b : int = (move a);

    struct ms {
      int a;
      double b;
    }

    let v : ms = (2, 3.5);
    let x : double = (move v.b);

    func test() -> double {
      x = 4.5;
      return v.b;
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
  auto a = interpreter.FindVariableById("main$a");
  EXPECT_EQ(wamon::AsIntVariable(a)->GetValueCategory(), wamon::Variable::ValueCategory::LValue);
  EXPECT_EQ(wamon::AsIntVariable(a)->GetValue(), 0);

  auto ret = interpreter.CallFunctionByName("main$test", {});
  EXPECT_EQ(ret->GetTypeInfo(), "double");
  EXPECT_DOUBLE_EQ(wamon::AsDoubleVariable(ret)->GetValue(), 0.0);
  auto x = interpreter.FindVariableById("main$x");
  EXPECT_DOUBLE_EQ(wamon::AsDoubleVariable(x)->GetValue(), 4.5);
}

TEST(interpreter, lambda) {
  wamon::Scanner scan;
  std::string str = R"(
    package main;

    let v : int = (3);

    func test() -> int {
      let v2 : int  = (2);
      let a : f(() -> int) = (lambda [v, v2] () -> int { return v + v2; });
      v2 = 10;
      v = 20;
      return call a:();
    }

    func test2() -> int {
      let v : int = (2);
      return call lambda [v] (int a) -> int  { return v + a; }:(10);
    }

    let v1 : int = (10);

    func test3() -> int {
      let lmd : f(() -> int) = lambda [move v1, ref v]() -> int { 
        v = v * 2;
        return v1 + v;
      };

      return call lmd:();
    }

    let v2 : int = 15;
    let v3 : f(() -> void) = lambda [ref v2] () -> void {
      ++v2;
      return;
    };

    func test4() -> void {
      let v4 : f(() -> void) = v3;
      call v4:();
      let v5 : f(() -> void) = move v3;
      call v5:();
      call v4:();
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

  auto ret = interpreter.CallFunctionByName("main$test", {});
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 5);

  EXPECT_EQ(interpreter.FindVariableById("main$v")->IsRValue(), false);

  ret = interpreter.CallFunctionByName("main$test2", {});
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 12);
  EXPECT_EQ(interpreter.FindVariableById("main$v")->IsRValue(), false);

  ret = interpreter.CallFunctionByName("main$test3", {});
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 50);

  ret = interpreter.FindVariableById("main$v");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 40);

  ret = interpreter.FindVariableById("main$v1");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 0);

  ret = interpreter.FindVariableById("main$v2");
  EXPECT_EQ(wamon::VarAs<int>(ret), 15);
  interpreter.CallFunctionByName("main$test4", {});
  ret = interpreter.FindVariableById("main$v2");
  EXPECT_EQ(wamon::VarAs<int>(ret), 18);
}

TEST(interpreter, new_expr) {
  wamon::Scanner scan;
  std::string str = R"(
    package main;

    let v : int = new int(2);

    struct test {
      string name;
    }

    let v2 : test = new test("chloro");
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

  auto ret = interpreter.FindVariableById("main$v");
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 2);
  ret = interpreter.FindVariableById("main$v2");
  EXPECT_EQ(ret->GetTypeInfo(), "main$test");
  EXPECT_EQ(wamon::AsStringVariable(wamon::AsStructVariable(ret)->GetDataMemberByName("name"))->GetValue(), "chloro");
}

TEST(interpreter, alloc) {
  wamon::Scanner scan;
  std::string script = R"(
    package main;

    let v : ptr(int) = alloc int(2);
    let v2 : ptr(int) = v;

    func test() -> void {
      *v = *v + 10;
      return;
    }

    func test2() -> void {
      dealloc v;
      return;
    }
  )";
  wamon::PackageUnit pu;
  auto tokens = scan.Scan(script);
  pu = wamon::Parse(tokens);
  pu = wamon::MergePackageUnits(std::move(pu));

  wamon::TypeChecker tc(pu);
  std::string reason;
  bool succ = tc.CheckAll(reason);
  EXPECT_EQ(succ, true) << reason;

  wamon::Interpreter interpreter(pu);
  auto it = interpreter.ExecExpression(tc, "main", "*v");
  EXPECT_EQ(wamon::AsIntVariable(it)->GetValue(), 2);
  interpreter.CallFunctionByName("main$test", {});
  it = interpreter.ExecExpression(tc, "main", "*v");
  EXPECT_EQ(wamon::AsIntVariable(it)->GetValue(), 12);
  it.reset();
  interpreter.CallFunctionByName("main$test2", {});
  EXPECT_THROW(interpreter.ExecExpression(tc, "main", "*v"), wamon::WamonException);
  EXPECT_THROW(interpreter.ExecExpression(tc, "main", "*v2"), wamon::WamonException);
}

TEST(interpreter, ref) {
  wamon::Scanner scan;
  std::string script = R"(
    package main;

    func test(int ref a, int b) -> void {
      a = a + 1;
      b = b + 1;
      return;
    }

    struct st {
      int a;
    }

    method st {
      func update(int ref b) -> void {
        b = b + self.a;
        return;
      }
    }

    let v : int = 2;
    let ref v2 : int = v;

    func test2() -> int {
      let v : int = 2;
      let tmp : st = (1);
      call tmp:update(v);
      return v;
    }

    func test3() -> void {
      v2 = 10;
      return;
    }
  )";
  wamon::PackageUnit pu;
  auto tokens = scan.Scan(script);
  pu = wamon::Parse(tokens);
  pu = wamon::MergePackageUnits(std::move(pu));

  wamon::TypeChecker tc(pu);
  std::string reason;
  bool succ = tc.CheckAll(reason);
  EXPECT_EQ(succ, true) << reason;

  wamon::Interpreter interpreter(pu);
  std::vector<std::shared_ptr<wamon::Variable>> params;
  params.push_back(
      wamon::VariableFactory(wamon::TypeFactory<int>::Get(), wamon::Variable::ValueCategory::LValue, "", interpreter));
  params.push_back(
      wamon::VariableFactory(wamon::TypeFactory<int>::Get(), wamon::Variable::ValueCategory::LValue, "", interpreter));
  wamon::AsIntVariable(params[0])->SetValue(1);
  wamon::AsIntVariable(params[1])->SetValue(1);
  auto hold_0 = params[0];
  auto hold_1 = params[1];
  interpreter.CallFunctionByName("main$test", std::move(params));
  EXPECT_EQ(wamon::AsIntVariable(hold_0)->GetValue(), 2);
  EXPECT_EQ(wamon::AsIntVariable(hold_1)->GetValue(), 1);
  auto v = interpreter.CallFunctionByName("main$test2", {});
  EXPECT_EQ(wamon::AsIntVariable(v)->GetValue(), 3);

  params.clear();
  params.push_back(
      wamon::VariableFactory(wamon::TypeFactory<int>::Get(), wamon::Variable::ValueCategory::RValue, "", interpreter));
  params.push_back(
      wamon::VariableFactory(wamon::TypeFactory<int>::Get(), wamon::Variable::ValueCategory::RValue, "", interpreter));
  EXPECT_THROW(interpreter.CallFunctionByName("main$test", std::move(params)), wamon::WamonException);

  interpreter.CallFunctionByName("main$test3", {});
  v = interpreter.FindVariableById("main$v");
  EXPECT_EQ(wamon::AsIntVariable(v)->GetValue(), 10);
}

TEST(interpreter, exec_expression) {
  wamon::Scanner scan;
  std::string script = R"(
    package main;

    let v : int = 10;

    let v2 : f(() -> int) = lambda [ref v]() -> int { return v + 10; };

    let v3 : double = 5.2;
  )";
  wamon::PackageUnit pu;
  auto tokens = scan.Scan(script);
  pu = wamon::Parse(tokens);
  pu = wamon::MergePackageUnits(std::move(pu));

  wamon::TypeChecker tc(pu);
  std::string reason;
  bool succ = tc.CheckAll(reason);
  EXPECT_EQ(succ, true) << reason;

  wamon::Interpreter interpreter(pu);
  auto ret = interpreter.ExecExpression(tc, "main", "v");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 10);
  ret = interpreter.ExecExpression(tc, "main", "call v2:()");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 20);
  ret = interpreter.ExecExpression(tc, "main", "v3 as int + v");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 15);
}

TEST(interpreter, enum) {
  wamon::Scanner scan;
  std::string script = R"(
    package main;

    enum Color {
      Red;
      Yellow;
      Blue;
    }

    let v : Color = enum Color:Red;

    enum Color2 {
      Red;
      White;
      Black;
    }
  )";
  wamon::PackageUnit pu;
  auto tokens = scan.Scan(script);
  pu = wamon::Parse(tokens);
  pu = wamon::MergePackageUnits(std::move(pu));

  wamon::TypeChecker tc(pu);
  std::string reason;
  bool succ = tc.CheckAll(reason);
  EXPECT_EQ(succ, true) << reason;

  wamon::Interpreter interpreter(pu);
  auto v = interpreter.FindVariableById("main$v");
  EXPECT_EQ(v->GetTypeInfo(), "main$Color");
  EXPECT_EQ(wamon::AsEnumVariable(v)->GetEnumItem(), "Red");

  EXPECT_THROW(interpreter.ExecExpression(tc, "main", "v = enum Color2:Black"), wamon::WamonException);
  EXPECT_THROW(interpreter.ExecExpression(tc, "main", "v = enum Color:Black"), wamon::WamonException);
}
