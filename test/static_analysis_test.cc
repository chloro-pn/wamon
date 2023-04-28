#include "gtest/gtest.h"

// for test
#define private public

#include "wamon/scanner.h"
#include "wamon/parser.h"
#include "wamon/exception.h"
#include "wamon/type_checker.h"
#include "wamon/static_analyzer.h"

TEST(static_analysis, base) {
  wamon::Scanner scan;
  std::string str = R"(
    package main;

    let global_var : int = (24);

    func calculate(int a, int b, string c) -> int {
      if (a == 20) {
        return b + a + global_var;
      } else {
        return a + b;
      }
    }
  )";
  auto tokens = scan.Scan(str);
  wamon::PackageUnit pu = wamon::Parse(tokens);
  wamon::StaticAnalyzer sa(pu);

  wamon::TypeChecker tc(sa);
  EXPECT_NO_THROW(tc.CheckAndRegisterGlobalVariable());
  EXPECT_NO_THROW(tc.CheckFunctions());
}

TEST(static_analysis, duplicate_name) {
  wamon::Scanner scan;
  std::string str = R"(
    package main;
    let global_var : int = (24);
    let global_var : string = ("hello world");
  )";

  auto tokens = scan.Scan(str);
  EXPECT_THROW(wamon::Parse(tokens), wamon::WamonExecption);

  str = R"(
    package main;

    func funca(int a) -> void {
      return 2;
    }

    func funca(string name) -> string {
      return name;
    }
  )";
  tokens = scan.Scan(str);
  EXPECT_THROW(wamon::Parse(tokens), wamon::WamonExecption);

  str = R"(
    package main;

    struct classa {
      int a;
    };

    struct classa {
      string name;
    };
  )";
  tokens = scan.Scan(str);
  EXPECT_THROW(wamon::Parse(tokens), wamon::WamonExecption);
}

TEST(static_analysis, type_dismatch) {
  wamon::Scanner scan;
  std::string str = R"(
    package main;

    let global_var : int = (24);

    func calculate(int a, int b, string c) -> int {
      // invalid + operand
      return a + c;
    }
  )";
  auto tokens = scan.Scan(str);
  wamon::PackageUnit pu = wamon::Parse(tokens);
  wamon::StaticAnalyzer sa(pu);

  wamon::TypeChecker tc(sa);
  EXPECT_NO_THROW(tc.CheckAndRegisterGlobalVariable());
  EXPECT_THROW(tc.CheckFunctions(), wamon::WamonExecption);

  str = R"(
    package main;

    let global_var : int = ("hello world");
  )";
  tokens = scan.Scan(str);
  pu = wamon::Parse(tokens);
  wamon::StaticAnalyzer sa2(pu);

  wamon::TypeChecker tc2(sa);
  EXPECT_THROW(tc2.CheckAndRegisterGlobalVariable(), wamon::WamonExecption);
}

TEST(static_analysis, construct_check) {
  wamon::Scanner scan;
  std::string str = R"(
    package main;

    struct mclass {
      ptr(int) a;
      string b;
      bool c;
    }

    let global_var : array(int, 3) = (2, 3, 4);
    let int_var : int = (2);
    let ptr_var : ptr(int) = (&int_var);
    let struct_var : mclass = (&int_var, "hello world", false);
  )";

  auto tokens = scan.Scan(str);
  wamon::PackageUnit pu = wamon::Parse(tokens);
  wamon::StaticAnalyzer sa(pu);
  wamon::TypeChecker tc(sa);
  EXPECT_NO_THROW(tc.CheckAndRegisterGlobalVariable());
  std::vector<std::string> strs = {
    R"(
      package main;
      let array_var : array(int, 3) = (2, 3);
    )",
    R"(
      package main;
      let array_var : array(int , 3) = ("hello world", 2, 3);
    )",
    R"(
      package main;
      let void_var : void = (2);
    )",
    R"(
      package main;
      let func_var : f(()) = (0);
    )",
    R"(
      package main;
      struct mclass {
        int a;
        int b;
      }

      let struct_var : mclass = ("hello world", 2);
    )",
  };
  for(auto str : strs) {
    auto tokens = scan.Scan(str);
    wamon::PackageUnit pu = wamon::Parse(tokens);
    wamon::StaticAnalyzer sa(pu);
    wamon::TypeChecker tc(sa);
    EXPECT_THROW(tc.CheckAndRegisterGlobalVariable(), wamon::WamonExecption);
  }
}