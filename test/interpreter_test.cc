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

  wamon::StaticAnalyzer sa(pu);
  wamon::TypeChecker tc(sa);
  tc.CheckTypes();
  tc.CheckAndRegisterGlobalVariable();
  tc.CheckStructs();
  tc.CheckFunctions();
  tc.CheckMethods();
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

    func add(int a, int b) -> int {
      return a + b;
    }

    let mydata : my_struct_name = (2, 3.5, "hello");
    let mystr : string = ("hello");
    let myptr : ptr(my_struct_name) = (&mydata);
    let mylen : int = (call mystr.len());
    let mylist : list(int) = (2, 3, 4);
    let myfunc : f((int, int) -> int) = (add);

    let myfunc2 : f((int, int) -> int) = (myfunc);
    let call_ret : int = (call myfunc2(2, 3));

    func update_list() -> int {
      mylist[1] = mylist[1] * 2;
      return mylist[1];
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
  // tc.CheckOperatorOverride();

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

  v = interpreter.FindVariableById("myfunc");
  EXPECT_EQ(v->GetTypeInfo(), "f((int, int) -> int)");
  v = interpreter.FindVariableById("myfunc2");
  EXPECT_EQ(v->GetTypeInfo(), "f((int, int) -> int)");

  v = interpreter.FindVariableById("call_ret");
  EXPECT_EQ(v->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(v)->GetValue(), 5);

  interpreter.EnterContext<wamon::RuntimeContextType::Function>();
  auto ret = interpreter.CallFunctionByName("update_list", {});
  interpreter.LeaveContext();
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

  wamon::StaticAnalyzer sa(pu);
  wamon::TypeChecker tc(sa);
  tc.CheckTypes();
  tc.CheckAndRegisterGlobalVariable();
  tc.CheckStructs();
  tc.CheckFunctions();
  tc.CheckMethods();
  wamon::Interpreter interpreter(pu);
  auto v = interpreter.FindVariableById("a");
  auto v2 = interpreter.FindVariableById("c");
  EXPECT_EQ(v->Compare(v2), false);

  v = interpreter.FindVariableById("b");
  v2 = interpreter.FindVariableById("d");
  EXPECT_EQ(v->Compare(v2), true);

  v = interpreter.FindVariableById("e");
  v2 = interpreter.FindVariableById("g");
  EXPECT_EQ(v->Compare(v2), false);

  v = interpreter.FindVariableById("h");
  v2 = interpreter.FindVariableById("i");
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

  wamon::StaticAnalyzer sa(pu);
  wamon::TypeChecker tc(sa);
  tc.CheckTypes();
  tc.CheckAndRegisterGlobalVariable();
  tc.CheckStructs();
  tc.CheckFunctions();
  tc.CheckMethods();
  wamon::Interpreter interpreter(pu);
  auto v = interpreter.FindVariableById("a");
  auto v2 = interpreter.FindVariableById("c");
  v->Assign(v2);
  EXPECT_EQ(wamon::AsIntVariable(v)->GetValue(), 3);

  v = interpreter.FindVariableById("b");
  v2 = interpreter.FindVariableById("d");
  v->Assign(v2);
  EXPECT_EQ(wamon::AsStringVariable(v)->GetValue(), "bob");

  v = interpreter.FindVariableById("e");
  v2 = interpreter.FindVariableById("g");
  v->Assign(v2);
  EXPECT_EQ(wamon::AsBoolVariable(wamon::AsStructVariable(v)->GetDataMemberByName("b"))->GetValue(), true);

  v = interpreter.FindVariableById("h");
  v2 = interpreter.FindVariableById("j");
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
  interpreter.EnterContext<wamon::RuntimeContextType::Method>();
  auto ret = interpreter.CallMethodByName(v, "get_age", {});
  interpreter.LeaveContext();
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 25);
}

TEST(interpreter, inner_type_method) {
  wamon::Scanner scan;
  std::string str = R"(
    package main;

    let v1 : string = ("hello");
    let v2 : list(int) = (2, 3, 4);

    func func1() -> int {
      return call v1.len();
    }

    func func2() -> byte {
      return call v1.at(0);
    }

    func func3() -> int {
      return call v2.size() + call v2.at(1);
      // 6
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
  interpreter.EnterContext<wamon::RuntimeContextType::Function>();
  auto ret = interpreter.CallFunctionByName("func1", {});
  interpreter.LeaveContext();
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 5);

  interpreter.EnterContext<wamon::RuntimeContextType::Function>();
  ret = interpreter.CallFunctionByName("func2", {});
  interpreter.LeaveContext();
  EXPECT_EQ(ret->GetTypeInfo(), "byte");
  EXPECT_EQ(wamon::AsByteVariable(ret)->GetValue(), 'h');

  interpreter.EnterContext<wamon::RuntimeContextType::Function>();
  ret = interpreter.CallFunctionByName("func3", {});
  interpreter.LeaveContext();
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 6);
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

      return call f1(0) + call f2(0) + call ms(-20);
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
  auto v = interpreter.FindVariableById("mycallable");
  auto a = interpreter.FindVariableById("v");
  std::vector<std::shared_ptr<wamon::Variable>> params;
  params.push_back(a);
  auto ret = interpreter.CallCallable(v, std::move(params));
  EXPECT_EQ(ret->GetTypeInfo(), "int");

  params.clear();
  ret = interpreter.CallFunctionByName("call_test", std::move(params));
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 31);
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

    let v1 : myint = (2);
    let v2 : myint = (3);

    operator + (myint a, myint b) -> myint {
      let tmp : myint = (a.a + b.a);
      return tmp;
    }

    func op_override2() -> int {
      return (v1 + v2).a;
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
  params.push_back(std::shared_ptr<wamon::StringVariable>(new wamon::StringVariable("hello ", "")));
  params.push_back(std::shared_ptr<wamon::StringVariable>(new wamon::StringVariable("world", "")));
  interpreter.EnterContext<wamon::RuntimeContextType::Function>();
  auto ret = interpreter.CallFunctionByName("stradd", std::move(params));
  interpreter.LeaveContext();
  EXPECT_EQ(ret->GetTypeInfo(), "string");
  EXPECT_EQ(wamon::AsStringVariable(ret)->GetValue(), "hello world");

  params.clear();
  params.push_back(std::shared_ptr<wamon::IntVariable>(new wamon::IntVariable(10, "")));
  params.push_back(std::shared_ptr<wamon::IntVariable>(new wamon::IntVariable(5, "")));
  auto tmp_params = params;
  interpreter.EnterContext<wamon::RuntimeContextType::Function>();
  ret = interpreter.CallFunctionByName("intminus", std::move(tmp_params));
  interpreter.LeaveContext();
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 5);

  tmp_params = params;
  interpreter.EnterContext<wamon::RuntimeContextType::Function>();
  ret = interpreter.CallFunctionByName("intmulti", std::move(tmp_params));
  interpreter.LeaveContext();
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 50);

  tmp_params = params;
  interpreter.EnterContext<wamon::RuntimeContextType::Function>();
  ret = interpreter.CallFunctionByName("intdivide", std::move(tmp_params));
  interpreter.LeaveContext();
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 2);

  tmp_params.clear();
  tmp_params.push_back(std::shared_ptr<wamon::IntVariable>(new wamon::IntVariable(10, "")));
  interpreter.EnterContext<wamon::RuntimeContextType::Function>();
  ret = interpreter.CallFunctionByName("intuoperator", std::move(tmp_params));
  interpreter.LeaveContext();
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), -10);

  tmp_params.clear();
  tmp_params.push_back(std::shared_ptr<wamon::BoolVariable>(new wamon::BoolVariable(true, "")));
  interpreter.EnterContext<wamon::RuntimeContextType::Function>();
  ret = interpreter.CallFunctionByName("boolnot", std::move(tmp_params));
  interpreter.LeaveContext();
  EXPECT_EQ(ret->GetTypeInfo(), "bool");
  EXPECT_EQ(wamon::AsBoolVariable(ret)->GetValue(), false);

  tmp_params.clear();
  interpreter.EnterContext<wamon::RuntimeContextType::Function>();
  ret = interpreter.CallFunctionByName("op_override", std::move(tmp_params));
  interpreter.LeaveContext();
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 5);

  tmp_params.clear();
  auto v1 = interpreter.FindVariableById("v1");
  auto v2 = interpreter.FindVariableById("v2");
  interpreter.EnterContext<wamon::RuntimeContextType::Function>();
  ret = interpreter.CallFunctionByName("op_override2", std::move(tmp_params));
  interpreter.LeaveContext();
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 5);
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
      return call Fibonacci(n - 1) + call Fibonacci(n - 2);
    }

    let v : int = (10);
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
  auto a = interpreter.FindVariableById("v");
  std::vector<std::shared_ptr<wamon::Variable>> params;
  params.push_back(a);
  auto ret = interpreter.CallFunctionByName("Fibonacci", std::move(params));
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 55);
}

TEST(interpreter, register_cpp_function) {
  wamon::Scanner scan;
  std::string str = R"(
    package main;

    func testfunc() -> int {
      let v2 : int = (call func111("hello"));
      return v2;
    }
  )";
  wamon::PackageUnit pu;
  auto tokens = scan.Scan(str);
  pu = wamon::Parse(tokens);

  wamon::Interpreter interpreter(pu);
  interpreter.RegisterCppFunctions(
      "func111",
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
        return std::make_shared<wamon::IntVariable>(static_cast<int>(len), "");
      });

  wamon::StaticAnalyzer sa(pu);
  wamon::TypeChecker tc(sa);
  tc.CheckTypes();
  tc.CheckAndRegisterGlobalVariable();
  tc.CheckStructs();
  tc.CheckFunctions();
  tc.CheckMethods();

  interpreter.EnterContext<wamon::RuntimeContextType::Function>();
  auto ret = interpreter.CallFunctionByName("testfunc", {});
  interpreter.LeaveContext();
  EXPECT_EQ(ret->GetTypeInfo(), "int");
  EXPECT_EQ(wamon::AsIntVariable(ret)->GetValue(), 5);
}