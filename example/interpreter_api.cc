#include <iostream>
#include <string>

#include "wamon/interpreter.h"
#include "wamon/parser.h"
#include "wamon/scanner.h"
#include "wamon/type_checker.h"
#include "wamon/variable.h"

/* output should be:
 *
 * call function teest1 get result : 11
 * call method get_b get result : world
 * call callable get result : 12
 */

int main() {
  std::string script = R"(
    package main;

    func test1(int a)  -> int {
      return a + 1;
    }

    struct test2 {
      int a;
      string b;
    }

    method test2 {
      func get_b() -> string {
        return self.b;
      }

      operator () (int c) -> int {
        return self.a + c;
      }
    }
    // 注意，v2是Callable对象，v不是Callable对象，但因为类型test2重载了()运算符，因此可以用来构造Callable对象。
    // Callable对象是具有函数类型的对象
    // 可以用原始函数、lambda表达式、重载了()运算符类型的对象、Callable对象来构造Callable对象，只要类型匹配。
    let v : test2 = (2, "hello");
    let v2 : f((int) -> int) = move v;
  )";

  wamon::Scanner scan;
  auto tokens = scan.Scan(script);
  auto pu = wamon::Parse(tokens);
  pu = wamon::MergePackageUnits(std::move(pu));

  wamon::TypeChecker tc(pu);
  std::string reason;
  bool succ = tc.CheckAll(reason);
  if (succ == false) {
    std::cerr << "check error : " << reason << std::endl;
    return -1;
  }

  wamon::Interpreter ip(pu);
  // 利用VariableFactory
  // API构造变量对象，需要指定类型、值型别、名字[为空则代表匿名]和packageunit，packageunit是当指定类型中包含结构体类型时从该包中查找对应的类型定义。
  auto v = wamon::VariableFactory(wamon::TypeFactory<int>::Get(), wamon::Variable::ValueCategory::RValue, "", pu);
  // 利用AsXXXVariable系列API将变量转化为对应类型并进行赋值。
  wamon::AsIntVariable(v)->SetValue(10);
  // 填充函数参数，这里的设计还不优雅，需要进行类型转换std::unique_ptr -> std::shared_ptr
  std::vector<std::shared_ptr<wamon::Variable>> params;
  params.push_back(std::shared_ptr<wamon::Variable>(std::move(v)));
  // 利用CallFunctionByName
  // API调用函数，注意经过符号定位和重命名之后，对全局变量、函数、结构体等进行索引的时候需要在对应名字前加上包名称$。
  auto ret = ip.CallFunctionByName("main$test1", std::move(params));
  std::cout << "call function teest1 get result : " << wamon::AsIntVariable(ret)->GetValue() << std::endl;

  // 利用TypeFactory API构造结构体对应的类型并指定其所属的Package
  auto struct_type = wamon::TypeFactory<WAMON_STRUCT_TYPE("test2")>::Get();
  wamon::SetScopeForStructType(struct_type, "main");
  // 构造该类型的变量
  v = wamon::VariableFactory(struct_type, wamon::Variable::ValueCategory::RValue, "", pu);
  auto v2 = std::shared_ptr<wamon::Variable>(std::move(v));
  wamon::AsStructVariable(v2)->DefaultConstruct();
  auto string_v =
      wamon::VariableFactory(wamon::TypeFactory<std::string>::Get(), wamon::Variable::ValueCategory::RValue, "", pu);
  wamon::AsStringVariable(string_v)->SetValue("world");
  // 结构体成员变量赋值
  wamon::AsStructVariable(v2)->UpdateDataMemberByName("b", std::move(string_v));
  // 利用CallMethodByName API调用get_b方法，注意，方法不需要包前缀修饰
  auto tmp = ip.CallMethodByName(v2, "get_b", {});
  std::cout << "call method get_b get result : " << wamon::AsStringVariable(tmp)->GetValue() << std::endl;
  // 利用FindVariableById API获取全局变量，注意，不要忘记包前缀修饰
  v2 = ip.FindVariableById("main$v2");

  v = wamon::VariableFactory(wamon::TypeFactory<int>::Get(), wamon::Variable::ValueCategory::RValue, "", pu);
  wamon::AsIntVariable(v)->SetValue(10);
  // 利用CallCallable API调用Callable对象
  v2 = ip.CallCallable(v2, {std::move(v)});
  std::cout << "call callable get result : " << wamon::AsIntVariable(v2)->GetValue() << std::endl;
  return 0;
}
