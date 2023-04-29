#pragma once

#include "wamon/ast.h"
#include "wamon/type.h"
#include "wamon/context.h"

#include <set>

namespace wamon {

class StaticAnalyzer;

inline void RegisterFuncParamsToContext(const std::vector<std::pair<std::unique_ptr<Type>, std::string>>& params, Context* func_context) {
  std::set<std::string> param_names;
  for(auto& each : params) {
    if (param_names.find(each.second) != param_names.end()) {
      throw WamonExecption("func {} has duplicate param name {}", func_context->AssertFuncContextAndGetFuncName(), each.second);
    }
    func_context->RegisterVariable(each.second, each.first->Clone());
  }
}

class TypeChecker {
 public:
  explicit TypeChecker(StaticAnalyzer& sa);

  void CheckAndRegisterGlobalVariable();

  // 对表达式树进行后序遍历，根据子节点的情况检测每个节点的类型是否合法
  std::unique_ptr<Type> GetExpressionType(Expression* expr) const;

  void CheckStatement(Statement* stmt);

  // 检测函数定义是否合法
  // 定义了返回类型，是否在所有分支预测上都返回了；
  // 定义了无返回类型（void），是否在任一分支预测上都没有return expr；
  void CheckFunctions();

  // 对函数体进行确定性返回分析
  void CheckDeterministicReturn(FunctionDef* func);

  void CheckDeterministicReturn(MethodDef* method);

  const StaticAnalyzer& GetStaticAnalyzer() const {
    return static_analyzer_;
  }

  StaticAnalyzer& GetStaticAnalyzer() {
    return static_analyzer_;
  }

 private:
  StaticAnalyzer& static_analyzer_;
};

}