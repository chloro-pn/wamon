#pragma once

#include <unordered_map>
#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <cassert>

#include "wamon/variable.h"

namespace wamon {

class Type;
class FuncCallExpr;
class TypeChecker;

class BuiltinFunctions {
 public:
  using BuiltInFuncType = std::function<std::shared_ptr<Variable>(std::vector<std::shared_ptr<Variable>>&&)>;

  BuiltinFunctions();

  static BuiltinFunctions& Instance() {
    static BuiltinFunctions obj;
    return obj;
  }

  bool Find(const std::string& name) {
    return builtin_funcs_.find(name) != builtin_funcs_.end();
  }

  std::unique_ptr<Type> TypeCheck(const std::string& name, const TypeChecker& tc, FuncCallExpr* call_expr);

  BuiltInFuncType& Get(const std::string& name) {
    assert(Find(name) == true);
    return builtin_funcs_[name];
  }

 private:
  std::unordered_map<std::string, BuiltInFuncType> builtin_funcs_;

  std::unique_ptr<Type> len_type_check(const TypeChecker& tc, FuncCallExpr* call_expr);
};

}