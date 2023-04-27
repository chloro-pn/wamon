#pragma once

#include "wamon/ast.h"
#include "wamon/type.h"

namespace wamon {

class StaticAnalyzer;

class TypeChecker {
 public:
  // 对表达式树进行后序遍历，根据子节点的情况检测每个节点的类型是否合法
  std::unique_ptr<Type> GetExpressionType(Expression* expr) const;

  void CheckStatement(Statement* stmt);

  // 检测函数定义是否合法
  // 定义了返回类型，是否在所有分支预测上都返回了；
  // 定义了无返回类型（void），是否在任一分支预测上都没有return expr；
  void CheckFunctions() {
    
  }

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