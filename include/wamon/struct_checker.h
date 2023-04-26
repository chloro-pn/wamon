#pragma once

#include "wamon/ast.h"
#include "wamon/type.h"

namespace wamon {

class StaticAnalyzer;

class StructChecker {
 public:
  explicit StructChecker(StaticAnalyzer& sa) : static_analyzer_(sa) {}
  // 检测自定义结构体之间是否有循环依赖存在
  // 检测方法定义是否合法
  void CheckStructs() {

  }

 private:
  StaticAnalyzer& static_analyzer_;
};

}