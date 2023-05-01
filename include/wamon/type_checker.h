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

  // 检测函数、结构体、方法、全局变量定义中所涉及的所有类型是否合法
  // 因为定义是顺序无关的，因此解析的时候无法判别类型是否被定义，因此需要在解析后的某个阶段进行全局的类型合法性校验
  // 这应该是类型检测的第一个阶段
  void CheckTypes();

  // 检测全局变量的定义语句是否合法，以及全局变量间是否有循环依赖存在
  // 这应该是类型检测的第二个阶段（因为表达式合法性检测依赖名字查找，需要全局范围的名字判断合法性）
  void CheckAndRegisterGlobalVariable();

  // 对表达式树进行后序遍历，根据子节点的情况检测每个节点的类型是否合法
  std::unique_ptr<Type> GetExpressionType(Expression* expr) const;

  void CheckStatement(Statement* stmt);

  // 检测函数定义是否合法，包括参数和返回值类型是否合法，函数块内每条语句是否合法，以及确定性返回的检测
  // 这应该是类型检测的第三个阶段
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

  bool IsDeterministicReturn(BlockStmt* basic_block);
};

}