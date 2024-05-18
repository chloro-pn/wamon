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

## 类型系统

##### wamon的类型系统如下：
 - 基本类型：
   - 内置类型
     - int
     - string
     - byte
     - double
     - bool
     - void
   - 结构体类型
 - 复合类型(类型加工器):
   - 指针类型 *       `ptr(int)`
   - 列表类型 [num]   `list(int)`
   - 函数类型 <-      `f((int, string) -> int)`

wamon为这些类型提供了以下内置方法（目前这里还没做多少，需要提供更多的方法）：
* string:len() -> int
* string:at() -> byte
* list(T):at(int) -> T
* list(T):size() -> int
* list(T):insert(T) -> void
* list(T):push_back(T) -> void
* list(T):pop_back() -> void

##### 类型转换
wamon是强类型的，通过双元运算符as进行类型转换工作，目前支持：
* int -> double
* double -> int
* int -> bool
* list(byte) -> string
* struct / struct trait -> struct trait

## 值型别、变量定义和new表达式
wamon有与c++相似的值型别的概念，不过是简化版本，wamon中的变量有两种值型别：左值和右值：
* move运算符可以将左值变量变成右值变量，与c++不同的是，wamon中的move是一个单元运算符。
* 变量定义表达式的格式：`let var_name : type = expr | (expr_list);`，let定义的变量是左值类型
* 也可以通过new表达式定义一个匿名的临时变量：`new type(expr_list)`，new定义的变量是右值类型

## register cpp functions
wamon提供了如下接口将cpp函数注册到wamon运行时中（不提供类型转换和匹配操作，需要用户自己实现），ct在语义分析阶段调用，用户应该在ct中检查传入的参数类型是否符合要求，如果不符合需要抛出异常。
ht在运行阶段调用。

```c++
  std::string RegisterCppFunctions(const std::string& name, BuiltinFunctions::CheckType ct,
                                   BuiltinFunctions::HandleType ht);
  // ...
  using HandleType = std::function<std::shared_ptr<Variable>(std::vector<std::shared_ptr<Variable>>&&)>;
  using CheckType = std::function<std::unique_ptr<Type>(const std::vector<std::unique_ptr<Type>>& params_type)>;
```

你也可以通过下面这个接口注册类型确定的内置函数：
```c++
void Interpreter::RegisterCppFunctions(const std::string& name, std::unique_ptr<Type> func_type,
                                       BuiltinFunctions::HandleType ht);
```
这种内置函数参与类型分析，可以被赋值给Callable对象，详情见example/register_cpp_function.cc


## 函数是一等公民

wamon中的函数可以被赋值给变量、可以作为参数和返回值传递，是wamon中的一等公民。

wamon通过Callable变量持有原始函数、lambda表达式、重载了()运算符类型的变量。

Callable变量是指具有函数类型的变量：

`let callable_obj : f((int) -> int) = (...); `

Callable变量可以像普通变量一样被复制、传递、作为参数和返回值。

Callable变量可以出现在函数调用表达式表达式中：

`call callable_obj:(4); `

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