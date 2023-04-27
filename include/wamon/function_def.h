#pragma once

#include <string>
#include <vector>
#include <memory>
#include <utility>

#include "wamon/type.h"
#include "wamon/ast.h"

namespace wamon {

class TypeChecker;
class FuncCallExpr;

class FunctionDef {
 public:
  friend std::unique_ptr<Type> CheckParamTypeAndGetResultTypeForFunction(const TypeChecker& tc, FuncCallExpr* call_expr);
  friend std::unique_ptr<Type> CheckAndGetFuncReturnType(const TypeChecker& tc, const FunctionDef* function, const FuncCallExpr* call_expr);

  explicit FunctionDef(const std::string& name) : name_(name) {

  }

  const std::string& GetFunctionName() const {
    return name_;
  }

  const std::unique_ptr<Type>& GetReturnType() const {
    return return_type_;
  }

  void AddParamList(std::unique_ptr<Type>&& type, const std::string& var) {
    param_list_.push_back({std::move(type), var});
  }

  void SetReturnType(std::unique_ptr<Type>&& type) {
    return_type_ = std::move(type);
  }

  void SetBlockStmt(std::unique_ptr<BlockStmt>&& block_stmt) {
    block_stmt_ = std::move(block_stmt);
  }

 private:
  std::string name_;
  std::vector<std::pair<std::unique_ptr<Type>, std::string>> param_list_;
  std::unique_ptr<Type> return_type_;
  std::unique_ptr<BlockStmt> block_stmt_;
};

}