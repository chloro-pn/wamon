## wamon

![](https://tokei.rs/b1/github/chloro-pn/wamon) ![](https://tokei.rs/b1/github/chloro-pn/wamon?category=files) ![Static Badge](https://img.shields.io/badge/c%2B%2B-20-blue)

一个c++实现的脚本语言，以c++为宿主环境运行。

## design

* wamon的首要目标是提供与c++丝滑的交互能力，所见即所得、便于调试、类型安全、低阅读门槛、跨平台等特性的优先级均高于追求性能，因此本项目不会使用过多编译优化技术。

* wamon的变量是值语义的，变量的生命周期与作用域规则与c++保持一致，wamon不包含gc。

* wamon是两阶段的：脚本文本通过词法分析与语法分析后，需要进行语义分析，最后才能执行。

* wamon是强类型的，不支持c++中的隐式类型转换。

* wamon的使用一般遵循以下格式：

```c++
  // wamon::Scanner负责词法分析，输入脚本文本，输出tokens
  wamon::Scanner scan;
  wamon::PackageUnit pu;
  wamon::PackageUnit pu2;
  auto tokens = scan.Scan(str);
  // wamon::Parse函数负责语法分析，输入tokens，输出PackageUnit，每次只能Parse一个Package
  pu = wamon::Parse(tokens);

  tokens = scan.Scan(math_str);
  pn2 = wamon::Parse(tokens);
  // wamon::MergePackageUnits负责将多个Package合并为一个Package，其中涉及重命名和符号定位等工作，即使是一个包也需要进行这一步。
  pu = wamon::MergePackageUnits(std::move(pu), std::move(pu2));

  // wamon::TypeChecker负责语义分析，其中包括类型分析与上下文相关的语句和表达式的合法性分析等等，这个名字之后需要改，有歧义。
  wamon::TypeChecker tc(pu);
  std::string reason;
  bool succ = tc.CheckAll(reason);
  EXPECT_EQ(succ, true) << reason;
  // 通过语义分析后，构造wamon::Interpreter，通过wamon::Interpreter提供的接口执行脚本
  wamon::Interpreter ip(pu);
```

## hello world
```c++
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

    func hello() -> string {
      return "hello world";
    }
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
  auto ret = ip.CallFunctionByName("main$hello", {});
  std::cout << wamon::AsStringVariable(ret)->GetValue() << std::endl;
  return 0;
}
```