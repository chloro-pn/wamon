#pragma once

#include "wamon/variable.h"
#include "wamon/package_unit.h"

#include <unordered_map>
#include <vector>

namespace wamon {

enum class RuntimeContextType {
  Global,
  Function,
  Method,
  Block, // 流程控制语句或者块语句对应的上下文
};

struct RuntimeContext {
  void RegisterVariable(const std::shared_ptr<Variable>& variable) {
    const auto& name = variable->GetName();
    symbol_table_.insert({name, std::move(variable)});
  }

  void RegisterVariable(std::unique_ptr<Variable>&& variable) {
    const auto& name = variable->GetName();
    symbol_table_.insert({name, std::shared_ptr<Variable>(std::move(variable))});
  }

  RuntimeContextType type_;
  std::unordered_map<std::string, std::shared_ptr<Variable>> symbol_table_;
};

// 解释器，负责执行ast，维护运行时栈
class Interpreter {
 public:
  explicit Interpreter(const PackageUnit& pu) : pu_(pu) {
    package_context_.type_ = RuntimeContextType::Global;
    // 将packge unit中的包变量进行求解并插入包符号表中
    const auto& vars = pu_.GetGlobalVariDefStmt();
    for(const auto& each : vars) {
      each->Execute(*this);
    }
  }

  RuntimeContext& GetCurrentContext() {
    if (runtime_stack_.empty() == true) {
      return package_context_;
    }
    return runtime_stack_.back();
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