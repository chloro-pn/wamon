#pragma once

#include <functional>
#include <unordered_map>
#include <vector>

#include "wamon/ast.h"
#include "wamon/builtin_functions.h"
#include "wamon/exception.h"
#include "wamon/inner_type_method.h"
#include "wamon/operator.h"
#include "wamon/package_unit.h"
#include "wamon/variable.h"

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
  Block,  // 流程控制语句或者块语句对应的上下文
};

struct RuntimeContext {
  void check_not_exist(const std::string& name) {
    if (symbol_table_.find(name) != symbol_table_.end()) {
      throw WamonExecption("runtimeContext symbol_table get dumplicate variable name {}", name);
    }
  }

  void check_exist(const std::string& name) {
    if (symbol_table_.find(name) == symbol_table_.end()) {
      throw WamonExecption("runtimeContext symbol_table get not exist variable name {}", name);
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
    std::string name = variable->GetName();
    check_not_exist(name);
    symbol_table_.insert({name, std::shared_ptr<Variable>(std::move(variable))});
  }

  void UpdateVariable(std::shared_ptr<Variable> variable) {
    const std::string& name = variable->GetName();
    check_exist(name);
    symbol_table_[name] = variable;
  }

  std::shared_ptr<Variable> FindVariable(const std::string& id_name) {
    auto it = symbol_table_.find(id_name);
    if (it == symbol_table_.end()) {
      return nullptr;
    }
    return it->second;
  }

  RuntimeContext() = default;

  RuntimeContext(const RuntimeContext&) = delete;
  RuntimeContext(RuntimeContext&& other) : type_(other.type_), symbol_table_(std::move(other.symbol_table_)) {}
  RuntimeContext& operator=(const RuntimeContext& other) = delete;
  RuntimeContext& operator=(RuntimeContext&& other) {
    type_ = other.type_;
    symbol_table_ = std::move(other.symbol_table_);
    return *this;
  }

  RuntimeContextType type_;
  std::unordered_map<std::string, std::shared_ptr<Variable>> symbol_table_;
};

// 解释器，负责执行ast，维护运行时栈，运算符执行
class Interpreter {
 public:
  friend class BlockStmt;
  friend class IfStmt;
  friend class ForStmt;
  friend class WhileStmt;

  explicit Interpreter(const PackageUnit& pu);

 private:
  template <RuntimeContextType type>
  void EnterContext() {
    auto rs = std::make_unique<RuntimeContext>();
    rs->type_ = type;
    runtime_stack_.push_back(std::move(rs));
  }

  void LeaveContext() {
    assert(runtime_stack_.empty() == false);
    runtime_stack_.pop_back();
  }

 public:
  RuntimeContext* GetCurrentContext() {
    if (runtime_stack_.empty() == true) {
      return &package_context_;
    }
    return runtime_stack_.back().get();
  }

  // 获取当前调用栈上最近的方法调用对象
  std::shared_ptr<Variable> GetSelfObject() { return FindVariableById("__self__"); }

  template <typename... VariableType>
  std::shared_ptr<Variable> CalculateOperator(Token op, const std::shared_ptr<VariableType>&... operand) {
    auto ret = Operator::Instance().Calculate(*this, op, operand...);
    if (ret == nullptr) {
      auto override_name = OperatorDef::CreateName(op, {operand...});
      auto func_def = GetPackageUnit().FindFunction(override_name);
      if (func_def != nullptr) {
        EnterContext<RuntimeContextType::Function>();
        ret = CallFunction(func_def, {operand...});
        LeaveContext();
        return ret;
      }
    } else {
      return ret;
    }
    throw WamonExecption("operator {} calculate error, handle not exist", GetTokenStr(op));
  }

  template <bool throw_if_not_found = true>
  std::shared_ptr<Variable> FindVariableById(const std::string& id_name) {
    for (auto it = runtime_stack_.rbegin(); it != runtime_stack_.rend(); ++it) {
      // 对于函数和方法栈，如果找不到则直接在全局作用域中查找，而不是继续向上查找
      if ((*it)->type_ == RuntimeContextType::Method || (*it)->type_ == RuntimeContextType::Function) {
        auto result = (*it)->FindVariable(id_name);
        if (result != nullptr) {
          return result;
        } else {
          break;
        }
      }
      auto result = (*it)->FindVariable(id_name);
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

  std::shared_ptr<Variable> CallFunction(const FunctionDef* function_def,
                                         std::vector<std::shared_ptr<Variable>>&& params);

  // call builtin funcs
  std::shared_ptr<Variable> CallFunction(const std::string& builtin_name,
                                         std::vector<std::shared_ptr<Variable>>&& params) {
    auto func = BuiltinFunctions::Instance().Get(builtin_name);
    if (func == nullptr) {
      throw WamonExecption("CallFunction error, {} not exist", builtin_name);
    }
    EnterContext<RuntimeContextType::Function>();
    auto ret = (*func)(std::move(params));
    LeaveContext();
    return ret->IsRValue() ? std::move(ret) : ret->Clone();
  }

  // todo : builtin funcs need type check
  std::shared_ptr<Variable> CallFunctionByName(const std::string& func_name,
                                               std::vector<std::shared_ptr<Variable>>&& params) {
    if (BuiltinFunctions::Instance().Find(func_name)) {
      return CallFunction(func_name, std::move(params));
    }
    return CallFunction(pu_.FindFunction(func_name), std::move(params));
  }

  std::shared_ptr<Variable> CallCallable(std::shared_ptr<Variable> callable,
                                         std::vector<std::shared_ptr<Variable>>&& params);

  std::shared_ptr<Variable> CallMethod(std::shared_ptr<Variable> obj, const MethodDef* method_def,
                                       std::vector<std::shared_ptr<Variable>>&& params);

  std::shared_ptr<Variable> CallMethod(std::shared_ptr<Variable> obj, const std::string& method_name,
                                       std::vector<std::shared_ptr<Variable>>&& params) {
    auto method = InnerTypeMethod::Instance().Get(obj, method_name);
    EnterContext<RuntimeContextType::Method>();
    auto ret = method(obj, std::move(params));
    LeaveContext();
    return ret->IsRValue() ? std::move(ret) : ret->Clone();
  }

  std::shared_ptr<Variable> CallMethodByName(std::shared_ptr<Variable> obj, const std::string& method_name,
                                             std::vector<std::shared_ptr<Variable>>&& params) {
    if (IsInnerType(obj->GetType())) {
      return CallMethod(obj, method_name, std::move(params));
    }
    auto type = obj->GetTypeInfo();
    auto method_def = pu_.FindStruct(type)->GetMethod(method_name);
    return CallMethod(obj, method_def, std::move(params));
  }

  std::string RegisterCppFunctions(const std::string& name, BuiltinFunctions::CheckType ct,
                                   BuiltinFunctions::HandleType ht) {
    return BuiltinFunctions::Instance().Register("wamon$" + name, std::move(ct), std::move(ht));
  }

  const PackageUnit& GetPackageUnit() const { return pu_; }

 private:
  // 这里使用vector模拟栈，因为需要对其进行遍历
  std::vector<std::unique_ptr<RuntimeContext>> runtime_stack_;
  // 包符号表
  RuntimeContext package_context_;
  // 解释执行的时候需要从package_unit_中查找函数、方法、类型等信息
  const PackageUnit& pu_;
};

}  // namespace wamon
