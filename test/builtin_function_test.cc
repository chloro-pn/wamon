#include "gtest/gtest.h"
#include "wamon/builtin_functions.h"
#include "wamon/interpreter.h"
#include "wamon/parser.h"
#include "wamon/scanner.h"
#include "wamon/static_analyzer.h"
#include "wamon/type_checker.h"
#include "wamon/variable.h"

TEST(builtin_function, context_stack) {
  using namespace wamon;
  std::string script = R"(
    package main;

    let cs : list(string) = ();

    struct ms {
      int a;
    }

    method ms {
      func func_method() -> void {
        call func2:();
        return;
      }
    }

    func func1() -> void {
      let tmp : ms = (2);
      call tmp:func_method();
      return;
    }

    func func2() -> void {
      call func3:();
      return;
    }

    func func3() -> void {
      {
        let lmd: f(()->void) = lambda [] () -> void { cs = call wamon::context_stack:(); return; };
        call lmd:();
      }
      return;
    }
  )";
  Scanner scan;
  auto tokens = scan.Scan(script);
  auto pu = Parse(tokens);
  pu = MergePackageUnits(std::move(pu));
  EXPECT_THROW(pu.RegisterCppFunctions("mytest", BuiltinFunctions::CheckType(), BuiltinFunctions::HandleType()),
               WamonException);

  TypeChecker tc(pu);
  std::string reason;
  EXPECT_EQ(tc.CheckAll(reason), true);

  Interpreter ip(pu);
  ip.CallFunctionByName("main$func1", {});
  auto v = ip.FindVariableById("main$cs");
  std::string dump_info =
      R"_(["__lambda_0main","main$func3","main$func2","main$ms::func_method","main$func1","global"])_";
  EXPECT_EQ(v->Print().dump(), dump_info);
}