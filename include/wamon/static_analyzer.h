#pragma once

#include <cassert>
#include <stack>

#include "wamon/ast.h"
#include "wamon/context.h"
#include "wamon/package_unit.h"
#include "wamon/type.h"

namespace wamon {

// 静态分析器，在词法分析和语法分析之后的第三个阶段，执行上下文相关的语义分析，包括类型诊断、定义声明规则诊断、语句合法性诊断等。
class StaticAnalyzer {
 public:
  explicit StaticAnalyzer(const PackageUnit& pu) : pu_(pu), global_context_(Context::ContextType::GLOBAL) {}

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

  std::unique_ptr<Type> GetTypeByName(const std::string& name, IdExpr::Type& type) const {
    for (auto it = context_stack_.rbegin(); it != context_stack_.rend(); ++it) {
      auto result = (*it)->GetTypeByName(name);
      if (result != nullptr) {
        type = IdExpr::Type::Variable;
        return result;
      }
    }
    auto result = global_context_.GetTypeByName(name);
    if (result != nullptr) {
      type = IdExpr::Type::Variable;
      return result;
    }
    const FunctionDef* fd = pu_.FindFunction(name);
    if (fd != nullptr) {
      type = IdExpr::Type::Function;
      return fd->GetType();
    }
    throw WamonExecption("GetTypeByName error, can't find name {}, maybe builtin function or invalid", name);
  }

  void RegisterFuncParamsToContext(const std::vector<std::pair<std::unique_ptr<Type>, std::string>>& params,
                                   Context* func_context) {
    std::set<std::string> param_names;
    for (auto& each : params) {
      if (param_names.find(each.second) != param_names.end()) {
        throw WamonExecption("func or method {} has duplicate param name {}",
                             func_context->AssertFuncContextAndGetFuncName(), each.second);
      }
      func_context->RegisterVariable(each.second, each.first->Clone());
    }
  }

  void RegisterCaptureIdsToContext(const std::vector<std::string>& ids, Context* func_context) {
    for (auto& each : ids) {
      IdExpr::Type type = IdExpr::Type::Invalid;
      auto id_type = GetTypeByName(each, type);
      if (type == IdExpr::Type::Invalid || type == IdExpr::Type::Function) {
        throw WamonExecption("StaticAnalyzer.RegisterFuncParamsToContext, invalid or function id name {}", each);
      }
      func_context->RegisterVariable(each, std::move(id_type));
    }
  }

  // 由于lambda的存在，函数作用域是可以嵌套的，因此这里不做限制，只需要解析的时候不在非全局作用域解析函数定义即可
  void Enter(std::unique_ptr<Context>&& c) {
    if (c->GetType() == Context::ContextType::GLOBAL) {
      throw WamonExecption("enter context error, the global context should be unique");
    }
    context_stack_.push_back(std::move(c));
  }

  void Leave() {
    assert(context_stack_.empty() == false);
    context_stack_.pop_back();
  }

  Context* GetCurrentContext() {
    if (context_stack_.empty() == false) {
      return context_stack_.back().get();
    }
    return &global_context_;
  }

  const std::string& CheckMethodContextAndGetTypeName() const {
    for (auto it = context_stack_.rbegin(); it != context_stack_.rend(); ++it) {
      if ((*it)->GetLevel() == 2) {
        continue;
      }
      if ((*it)->GetLevel() == 1) {
        return (*it)->AssertMethodContextAndGetTypeName();
      }
    }
    throw WamonExecption("check method context error");
  }

  void CheckForOrWhileContext() {
    for (auto it = context_stack_.rbegin(); it != context_stack_.rend(); ++it) {
      if ((*it)->GetLevel() == 1) {
        throw WamonExecption("check for or while context error");
      }
      Context* cur = (*it).get();
      if (cur->GetLevel() == 2 &&
          (cur->GetType() == Context::ContextType::FOR_BLOCK || cur->GetType() == Context::ContextType::WHILE_BLOCK)) {
        return;
      }
    }
    throw WamonExecption("check for or while context error");
  }

  std::unique_ptr<Type> CheckFuncOrMethodAndGetReturnType() {
    for (auto it = context_stack_.rbegin(); it != context_stack_.rend(); ++it) {
      if ((*it)->GetLevel() == 2) {
        continue;
      }
      if ((*it)->GetType() == Context::ContextType::FUNC) {
        auto func_name = (*it)->AssertFuncContextAndGetFuncName();
        auto func_def = pu_.FindFunction(func_name);
        return func_def->GetReturnType()->Clone();
      }
      if ((*it)->GetType() == Context::ContextType::METHOD) {
        auto type_name = (*it)->AssertMethodContextAndGetTypeName();
        auto method_name = (*it)->AssertMethodContextAndGetMethodName();
        auto method_def = pu_.FindTypeMethod(type_name, method_name);
        return method_def->GetReturnType()->Clone();
      }
    }
    throw WamonExecption("check func or method error");
  }

  const std::vector<std::unique_ptr<VariableDefineStmt>>& GetGlobalVarDefStmt() const {
    return pu_.GetGlobalVariDefStmt();
  }

  const std::unordered_map<std::string, std::unique_ptr<FunctionDef>>& GetFunctions() const {
    return pu_.GetFunctions();
  }

  const PackageUnit& GetPackageUnit() const { return pu_; }

 private:
  const PackageUnit& pu_;
  Context global_context_;
  std::vector<std::unique_ptr<Context>> context_stack_;
};

}  // namespace wamon
