#pragma once

#include "wamon/variable.h"
#include "wamon/package_unit.h"
#include "wamon/exception.h"
#include "wamon/ast.h"
#include "wamon/builtin_functions.h"

#include <unordered_map>
#include <vector>
#include <functional>

namespace wamon {

enum class RuntimeContextType {
  Global,
  Function,
  Callable,
  Method,
  FOR,
  IF,
  ELSE,
  WHILE,
  Block, // 流程控制语句或者块语句对应的上下文
};

struct RuntimeContext {
  void check_not_exist(const std::string& name) {
    if (symbol_table_.find(name) != symbol_table_.end()) {
      throw WamonExecption("runtimeContext symbol_table get dumplicate variable name {}", name);
    }
  }
  void RegisterVariable(std::shared_ptr<Variable> variable) {
    const auto& name = variable->GetName();
    check_not_exist(name);
    symbol_table_.insert({name, variable});
  }

  // 方法调用里，调用者在方法栈中注册为__self__
  void RegisterVariable(const std::shared_ptr<Variable>& variable, const std::string& tmp_name) {
    check_not_exist(tmp_name);
    symbol_table_.insert({tmp_name, variable});
  }

  void RegisterVariable(std::unique_ptr<Variable>&& variable) {
    const auto& name = variable->GetName();
    check_not_exist(name);
    symbol_table_.insert({name, std::shared_ptr<Variable>(std::move(variable))});
  }

  std::shared_ptr<Variable> FindVariable(const std::string& id_name) {
    auto it = symbol_table_.find(id_name);
    if (it == symbol_table_.end()) {
      return nullptr;
    }
    return it->second;
  }

  RuntimeContextType type_;
  std::unordered_map<std::string, std::shared_ptr<Variable>> symbol_table_;
};

// 解释器，负责执行ast，维护运行时栈，运算符执行
class Interpreter {
 public:
  explicit Interpreter(const PackageUnit& pu);

  template <RuntimeContextType type>
  void EnterContext() {
    RuntimeContext rs;
    rs.type_ = type;
    runtime_stack_.push_back(std::move(rs));
  }

  void LeaveContext() {
    assert(runtime_stack_.empty() == false);
    runtime_stack_.pop_back();
  }

  RuntimeContext& GetCurrentContext() {
    if (runtime_stack_.empty() == true) {
      return package_context_;
    }
    return runtime_stack_.back();
  }

  // 获取当前调用栈上最近的方法调用对象
  std::shared_ptr<Variable> GetSelfObject() {
    return FindVariableById("__self__");
  }

  std::shared_ptr<Variable> CalculateOperator(Token op, const std::shared_ptr<Variable>& left, const std::shared_ptr<Variable>& right);

  std::shared_ptr<Variable> CalculateOperator(Token op, const std::shared_ptr<Variable>& operand);

  template <bool throw_if_not_found = true>
  std::shared_ptr<Variable> FindVariableById(const std::string& id_name) {
    for(auto it = runtime_stack_.rbegin(); it != runtime_stack_.rend(); ++it) {
      auto result = it->FindVariable(id_name);
      if (result != nullptr) {
        return result;
      }
    }
    auto result = package_context_.FindVariable(id_name);
    if (result == nullptr && throw_if_not_found == true) {
      throw WamonExecption("Interpreter.FindVariableById error, not found {}", id_name);
    }
    return result;
  }

  std::shared_ptr<Variable> CallFunction(const FunctionDef* function_def, std::vector<std::shared_ptr<Variable>>&& params);

  // call builtin funcs
  std::shared_ptr<Variable> CallFunction(const std::string& builtin_name, std::vector<std::shared_ptr<Variable>>&& params) {
    auto func = BuiltinFunctions::Instance().Get(builtin_name);
    return func(std::move(params));
  }

  // todo : builtin funcs need type check
  std::shared_ptr<Variable> CallFunctionByName(const std::string& func_name, std::vector<std::shared_ptr<Variable>>&& params) {
    if (BuiltinFunctions::Instance().Find(func_name)) {
      auto func = BuiltinFunctions::Instance().Get(func_name);
      return func(std::move(params));
    }
    return CallFunction(pu_.FindFunction(func_name), std::move(params));
  }

  std::shared_ptr<Variable> CallCallable(std::shared_ptr<Variable> callable, std::vector<std::shared_ptr<Variable>>&& params);

  std::shared_ptr<Variable> CallMethod(std::shared_ptr<Variable> obj, const MethodDef* method_def, std::vector<std::shared_ptr<Variable>>&& params);

  std::shared_ptr<Variable> CallMethodByName(std::shared_ptr<Variable> obj, const std::string& method_name, std::vector<std::shared_ptr<Variable>>&& params) {
    auto type = obj->GetTypeInfo();
    auto method_def = pu_.FindStruct(type)->GetMethod(method_name);
    return CallMethod(obj, method_def, std::move(params));
  }

  const PackageUnit& GetPackageUnit() const {
    return pu_;
  }

 private:
  // 这里使用vector模拟栈，因为需要对其进行遍历
  std::vector<RuntimeContext> runtime_stack_;
  // 包符号表
  RuntimeContext package_context_;
  // 解释执行的时候需要从package_unit_中查找函数、方法、类型等信息
  const PackageUnit& pu_;
};

}