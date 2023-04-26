#include <stack>
#include <cassert>

#include "wamon/context.h"
#include "wamon/ast.h"
#include "wamon/type.h"
#include "wamon/type_checker.h"
#include "wamon/package_unit.h"

namespace wamon {

// 静态分析器，在词法分析和语法分析之后的第三个阶段，执行上下文相关的语义分析，包括类型诊断、定义声明规则诊断、语句合法性诊断等。
class StaticAnalyzer {
 public:
  explicit StaticAnalyzer(const PackageUnit& pu):pu_(pu), global_context_(Context::ContextType::GLOBAL) {}

  const std::string& AssertMethodContextAndGetTypeName() {
    if (context_stack_.empty()) {
      throw WamonExecption("need method context but in global context");
    }
    return context_stack_.top()->AssertMethodContextAndGetTypeName();
  }

  // 直接在全局函数表中查找函数
  const FunctionDef* FindFunction(const std::string& fname) const {
    auto func = pu_.FindFunction(fname);
    if (func == nullptr) {
      throw WamonExecption("find function {} error, not exist", fname);
    }
    return func;
  }

  // 在类型type_name中查找名字为method_name的方法
  const MethodDef* FindTypeMethod(const std::string& type_name, const std::string& method_name) const {
    return pu_.FindTypeMethod(type_name, method_name);
  }

  std::unique_ptr<Type> GetDataMemberType(const std::string& type_name, const std::string& field_name) const {
    return pu_.GetDataMemberType(type_name, field_name);
  }

  std::unique_ptr<Type> GetTypeByName(const std::string& name) const {
    std::unique_ptr<Type> result;
    std::vector<std::unique_ptr<Context>> tmp;
    StaticAnalyzer* mutable_self = const_cast<StaticAnalyzer*>(this);
    while(context_stack_.empty() == false) {
      result = context_stack_.top()->GetTypeByName(name);
      if (result != nullptr) {
        break;
      }
      tmp.push_back(std::move(mutable_self->context_stack_.top()));
      mutable_self->context_stack_.pop();
    }
    for(auto& each : tmp) {
      mutable_self->context_stack_.push(std::move(each));
    }
    if (result != nullptr) {
      return result;
    }
    return global_context_.GetTypeByName(name);
  }

  void Enter(std::unique_ptr<Context>&& c) {
    context_stack_.push(std::move(c));
  }

  void Leave() {
    assert(context_stack_.empty() == false);
    context_stack_.pop();
  }

  Context* GetCurrentContext() {
    if (context_stack_.empty() == false) {
      return context_stack_.top().get();
    }
    return &global_context_;
  }
  
 private:
  const PackageUnit& pu_;
  Context global_context_;
  std::stack<std::unique_ptr<Context>> context_stack_;
};

}