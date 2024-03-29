## wamon

![](https://tokei.rs/b1/github/chloro-pn/wamon) ![](https://tokei.rs/b1/github/chloro-pn/wamon?category=files) ![Static Badge](https://img.shields.io/badge/c%2B%2B-20-blue)

一个c++实现的脚本语言，以c++为宿主环境运行。

## hello world
```c++
#include "wamon/interpreter.h"
#include "wamon/type_checker.h"
#include "wamon/scanner.h"
#include "wamon/parser.h"
#include "wamon/variable.h"

#include <string>
#include <iostream>

int main() {
  std::string script =  R"(
    package main;

    func hello() -> string {
      return "hello world";
    }
  )";

  wamon::Scanner scanner;
  auto tokens = scanner.Scan(script);
  wamon::PackageUnit package_unit = wamon::Parse(tokens);
  wamon::TypeChecker type_checker(package_unit);

  std::string reason;
  bool succ = type_checker.CheckAll(reason);
  if (succ == false) {
    std::cerr << "type check error : " << reason << std::endl;
    return -1;
  }
  wamon::Interpreter ip(package_unit);
  auto ret = ip.CallFunctionByName("hello", {});
  std::cout << wamon::AsStringVariable(ret)->GetValue() << std::endl;
  return 0;
}
```