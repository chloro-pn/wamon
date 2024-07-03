#include "wamon/variable.h"

#include <string>
#include <vector>

#include "catch2/catch_test_macros.hpp"
#include "wamon/interpreter.h"
#include "wamon/parser.h"
#include "wamon/scanner.h"
#include "wamon/type_checker.h"
#include "wamon/var_as.h"

TEST_CASE("variable", "ref_extend_lifecycle") {
  std::string script = R"(
    package main;

    let count : int = 0;

    struct person {
      int a;
      string name;
    }

    method person {
      destructor() {
        ++count;
      }
    }

    func test() -> void {
      let v : ptr(person) = alloc person(2, "chloro");
      let ref v2 : person = *v;
      dealloc v;
    }

  )";

  using namespace wamon;
  Scanner scan;
  auto tokens = scan.Scan(script);
  auto pu = Parse(tokens);
  pu = MergePackageUnits(std::move(pu));
  TypeChecker tc(pu);
  std::string reason;
  bool succ = tc.CheckAll(reason);
  REQUIRE(succ);
  Interpreter ip(pu);
  ip.CallFunctionByName("main$test", {});
  auto v = ip.FindVariableById("main$count");
  REQUIRE(VarAs<int>(v) == 1);
}