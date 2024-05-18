#include <iostream>
#include <string>

#include "wamon/interpreter.h"
#include "wamon/parser.h"
#include "wamon/scanner.h"
#include "wamon/type_checker.h"
#include "wamon/variable.h"

int main() {
  std::string script = R"(
    package main;

    struct trait my_trait {
      int age;
      string name;
    }

    method my_trait {
      func GetAge() -> int;
      func GetName() -> string;
    }

    struct ms {
      int age;
      string name;
      int is_man;
    }

    method ms {
      func GetAge() -> int {
        return self.age;
      }

      func GetName() -> string {
        return self.name;
      }

      func IsMan() -> bool {
        if (self.is_man == 0) {
          return true;
        }
        return false;
      }
    }

    let v : my_trait = new ms(24, "chloro", 0) as my_trait;
  )";

  wamon::Scanner scanner;
  auto tokens = scanner.Scan(script);
  wamon::PackageUnit package_unit = wamon::Parse(tokens);
  package_unit = wamon::MergePackageUnits(std::move(package_unit));
  wamon::TypeChecker type_checker(package_unit);

  std::string reason;
  bool succ = type_checker.CheckAll(reason);
  if (succ == false) {
    std::cerr << "type check error : " << reason << std::endl;
    return -1;
  }
  wamon::Interpreter ip(package_unit);
  auto v = ip.FindVariableById("main$v");
  auto name = ip.CallMethodByName(v, "GetName", {});
  std::cout << wamon::AsStringVariable(name)->GetValue() << std::endl;
  return 0;
}
