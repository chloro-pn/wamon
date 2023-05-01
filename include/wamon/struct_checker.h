#pragma once

#include "wamon/ast.h"
#include "wamon/type.h"
#include "wamon/topological_sort.h"

namespace wamon {

class StaticAnalyzer;

class StructChecker {
 public:
  explicit StructChecker(StaticAnalyzer& sa) : static_analyzer_(sa) {}
  
  void CheckStructs();

 private:
  StaticAnalyzer& static_analyzer_;
};

}