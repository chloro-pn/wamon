#pragma once

#include <string>
#include <vector>
#include <memory>
#include <utility>

#include "wamon/type.h"
#include "wamon/ast.h"
#include "wamon/function_def.h"

namespace wamon {

class TypeChecker;
class FuncCallExpr;

class MethodDef {
 public:
  friend std::unique_ptr<Type> CheckParamTypeAndGetResultTypeForFunction(const TypeChecker& tc, FuncCallExpr* call_expr);
  friend std::unique_ptr<Type> CheckAndGetMethodReturnType(const TypeChecker& tc, const MethodDef* method, const FuncCallExpr* call_expr);

  MethodDef(const std::string& tname, const std::string& mname) : type_name_(tname), method_name_(mname) {

  }

  MethodDef(const std::string& tname, std::unique_ptr<FunctionDef>&& fd) : type_name_(tname), method_name_(fd->GetFunctionName()) {

  }

  const std::string& GetMethodName() const {
    return method_name_;
  }

  const std::string& GetTypeName() const {
    return type_name_;
  }

  const std::unique_ptr<Type>& GetReturnType() const {
    return return_type_;
  }

  const std::vector<std::pair<std::unique_ptr<Type>, std::string>>& GetParamList() const {
    return param_list_;
  }

  const std::unique_ptr<BlockStmt>& GetBlockStmt() const {
    return block_stmt_;
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
  std::string type_name_;
  std::string method_name_;
  std::vector<std::pair<std::unique_ptr<Type>, std::string>> param_list_;
  std::unique_ptr<Type> return_type_;
  std::unique_ptr<BlockStmt> block_stmt_;
};

using methods_def = std::vector<std::unique_ptr<MethodDef>>;

}