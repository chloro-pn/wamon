#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "wamon/ast.h"
#include "wamon/function_def.h"
#include "wamon/parameter_list_item.h"
#include "wamon/type.h"

namespace wamon {

class TypeChecker;
class FuncCallExpr;
class OperatorDef;

class MethodDef {
 public:
  friend std::unique_ptr<Type> CheckParamTypeAndGetResultTypeForFunction(const TypeChecker& tc,
                                                                         FuncCallExpr* call_expr);
  friend std::unique_ptr<Type> CheckAndGetMethodReturnType(const TypeChecker& tc, const MethodDef* method,
                                                           const MethodCallExpr* call_expr);
  friend std::unique_ptr<MethodDef> OperatorOverrideToMethod(const std::string& type_name,
                                                             std::unique_ptr<OperatorDef>&& op);

  MethodDef(const std::string& tname, const std::string& mname) : type_name_(tname), method_name_(mname) {}

  MethodDef(const std::string& tname, std::unique_ptr<FunctionDef>&& fd)
      : type_name_(tname), method_name_(fd->GetFunctionName()) {
    param_list_ = std::move(fd->param_list_);
    return_type_ = std::move(fd->return_type_);
    block_stmt_ = std::move(fd->block_stmt_);
  }

  const std::string& GetMethodName() const { return method_name_; }

  const std::string& GetTypeName() const { return type_name_; }

  std::unique_ptr<Type> GetType() const {
    std::vector<std::unique_ptr<Type>> param_types;
    for (auto& each : param_list_) {
      param_types.push_back(each.type->Clone());
    }
    return std::make_unique<FuncType>(std::move(param_types), return_type_->Clone());
  }

  const std::unique_ptr<Type>& GetReturnType() const { return return_type_; }

  const std::vector<ParameterListItem>& GetParamList() const { return param_list_; }

  const std::unique_ptr<BlockStmt>& GetBlockStmt() const { return block_stmt_; }

  void AddParamList(ParameterListItem&& item) { param_list_.push_back(std::move(item)); }

  void SetReturnType(std::unique_ptr<Type>&& type) { return_type_ = std::move(type); }

  void SetBlockStmt(std::unique_ptr<BlockStmt>&& block_stmt) { block_stmt_ = std::move(block_stmt); }

  bool IsDeclaration() const { return block_stmt_ == nullptr; }

 private:
  std::string type_name_;
  std::string method_name_;
  std::vector<ParameterListItem> param_list_;
  std::unique_ptr<Type> return_type_;
  std::unique_ptr<BlockStmt> block_stmt_;
};

using methods_def = std::vector<std::unique_ptr<MethodDef>>;

}  // namespace wamon
