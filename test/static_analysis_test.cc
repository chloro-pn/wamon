#include "gtest/gtest.h"

#include <set>

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

    let callable_obj : f((int, int, string) -> int) = (calculate);
  )";
  auto tokens = scan.Scan(str);
  wamon::PackageUnit pu = wamon::Parse(tokens);
  wamon::StaticAnalyzer sa(pu);

  wamon::TypeChecker tc(sa);
  EXPECT_NO_THROW(tc.CheckAndRegisterGlobalVariable());
  EXPECT_NO_THROW(tc.CheckFunctions());
}

TEST(static_analysis, globalvar_dependent) {
  wamon::Scanner scan;
  std::string str = R"(
    package main;

    let global_var : int = (vv);

    let vv : int = (0);
  )";
  auto tokens = scan.Scan(str);
  wamon::PackageUnit pu = wamon::Parse(tokens);
  wamon::StaticAnalyzer sa(pu);

  wamon::TypeChecker tc(sa);
  EXPECT_THROW(tc.CheckAndRegisterGlobalVariable(), wamon::WamonExecption);
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

TEST(static_analysis, return_type) {
  std::string str = R"(
    package main;

    func test() -> int {
      let i : int = (0);
      for(i = 0; i < 10; i = i + 1) {
        if (i == 5) {
          return;
        }
      }
    }
  )";
  wamon::Scanner scan;
  std::vector<std::string> strs = {
    R"(
        package main;
        func test() -> int {
          let i : int = (0);
          if (i == 5) {
            return;
          }
        }
    )",
    R"(
      package main;
      func test() -> int {
        while(true) {
          return "hello world";
        }
      }
    )",
    R"(
      package main;
      func test() -> void {
        {
          {
            {
              return 0;
            }
          }
        }
      }
    )",
    R"(
      package main;
      func test() -> void {
        return 0;
      }
    )",
  };
  for(auto str : strs) {
    auto tokens = scan.Scan(str);
    wamon::PackageUnit pu = wamon::Parse(tokens);
    wamon::StaticAnalyzer sa(pu);
    wamon::TypeChecker tc(sa);
    tc.CheckAndRegisterGlobalVariable();
    EXPECT_THROW(tc.CheckFunctions(), wamon::WamonExecption);
  }
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

TEST(static_analysis, deterministic_return) {
  wamon::Scanner scan;
  std::vector<std::string> strs = {
    R"(
      package main;
      func test() -> int {
        if (true) {
          return 0;
        }
      }
    )",
    R"(
      package main;
      func test() -> int {
        while(true) {
          return 0;
        }
      }
    )",
    R"(
      package main;
      func test() -> int {
        if (true) {
          return 0; 
        } else {
          let a : int = (2);
        }
      }
    )",
    R"(
      package main;
      func test() -> int {
        let a : int = (2);
        if (true) {
          let b : int = (3);
          if (a == 0) {
            return b;
          } else {
            a = b;
          }
        } else {
          return a;
        }
      }
    )",
  };
  for(auto str : strs) {
    auto tokens = scan.Scan(str);
    wamon::PackageUnit pu = wamon::Parse(tokens);
    wamon::StaticAnalyzer sa(pu);
    wamon::TypeChecker tc(sa);
    EXPECT_THROW(tc.CheckFunctions(), wamon::WamonDeterministicReturn);
  }

  strs = {
    R"(
      package main;
      func test() -> int {
        {
          {
            {
              return 0;
            }
          }
        }
      }
    )",
    R"(
      package main;
      func test() -> int {
        if (true) {
          return 0;
        }
        return 1;
      }
    )",
    R"(
      package main;
      func test() -> int {
        while(true) {
          return 0;
        }
        return 1;
      }
    )",
    R"(
      package main;
      func test() -> int {
        if (true) {
          return 0; 
        } else {
          let a : int = (2);
          return 1;
        }
      }
    )",
    R"(
      package main;
      func test() -> int {
        let a : int = (2);
        if (true) {
          let b : int = (3);
          if (a == 0) {
            return b;
          } else {
            a = b;
            return a;
          }
        } else {
          return a;
        }
      }
    )",
  };
  for(auto str : strs) {
    auto tokens = scan.Scan(str);
    wamon::PackageUnit pu = wamon::Parse(tokens);
    wamon::StaticAnalyzer sa(pu);
    wamon::TypeChecker tc(sa);
    EXPECT_NO_THROW(tc.CheckFunctions());
  }
}

TEST(static_analysis, struct_dependent_info) {
  wamon::Scanner scan;
  std::string str = R"(
    package main;

    struct sa {
      int a;
      double b;
    }

    struct sb {
      sa a;
      ptr(double) b;
      f((int, double) -> string) c;
      array(byte, 3) d;
      array(array(array(bool, 3), 3), 3) e;
    }
  )";
  auto tokens = scan.Scan(str);
  wamon::PackageUnit pu = wamon::Parse(tokens);
  const wamon::StructDef* struct_def = pu.FindStruct("sb");
  auto dependents = struct_def->GetDependent();
  std::set<std::string> should_be = {
    "sa",
    "byte",
    "bool",
  };
  EXPECT_TRUE(dependents == should_be);
}

TEST(static_analysis, struct_dependent_check) {
  wamon::Scanner scan;
  std::string str = R"(
    package main;

    struct sa {
      int a;
      double b;
    }

    struct sb {
      sa a;
      double b;
    }
  )";
  auto tokens = scan.Scan(str);
  wamon::PackageUnit pu = wamon::Parse(tokens);
  wamon::StaticAnalyzer sa(pu);

  wamon::TypeChecker tc(sa);
  EXPECT_NO_THROW(tc.CheckStructs());

  str = R"(
    package main;

    struct sa {
      sb a;
      double b;
    }

    struct sb {
      sa a;
      double b;
    }
  )";

  tokens = scan.Scan(str);
  pu = wamon::Parse(tokens);
  wamon::StaticAnalyzer sa2(pu);
  wamon::TypeChecker tc2(sa2);
  EXPECT_THROW(tc2.CheckStructs(), wamon::WamonExecption);
}