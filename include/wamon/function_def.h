#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "wamon/capture_id_item.h"
#include "wamon/parameter_list_item.h"
#include "wamon/type.h"

namespace wamon {

class TypeChecker;
class FuncCallExpr;
class OperatorDef;
class BlockStmt;

class FunctionDef {
 public:
  friend std::unique_ptr<Type> CheckParamTypeAndGetResultTypeForFunction(const TypeChecker& tc,
                                                                         FuncCallExpr* call_expr);
  friend std::unique_ptr<Type> CheckAndGetFuncReturnType(const TypeChecker& tc, const FunctionDef* function,
                                                         const FuncCallExpr* call_expr);
  friend std::unique_ptr<FunctionDef> OperatorOverrideToFunc(std::unique_ptr<OperatorDef>&& op);
  friend class TypeChecker;
  friend class Interpreter;
  friend class MethodDef;

  explicit FunctionDef(const std::string& name) : name_(name) {}

  void SetFunctionName(const std::string& name) { name_ = name; }

  const std::string& GetFunctionName() const { return name_; }

  const std::unique_ptr<Type>& GetReturnType() const { return return_type_; }

  std::unique_ptr<Type> GetType() const {
    std::vector<std::unique_ptr<Type>> param_types;
    for (auto& each : param_list_) {
      param_types.push_back(each.type->Clone());
    }
    return std::make_unique<FuncType>(std::move(param_types), return_type_->Clone());
  }

  void AddParamList(ParameterListItem&& item) { param_list_.push_back(std::move(item)); }

  const std::vector<ParameterListItem>& GetParamList() const { return param_list_; }

  void SetReturnType(std::unique_ptr<Type>&& type) { return_type_ = std::move(type); }

  void SetBlockStmt(std::unique_ptr<BlockStmt>&& block_stmt);

  const std::unique_ptr<BlockStmt>& GetBlockStmt() const { return block_stmt_; }

  void SetCaptureIds(std::vector<CaptureIdItem>&& ids) { capture_ids_ = std::move(ids); }

  const std::vector<CaptureIdItem>& GetCaptureIds() const { return capture_ids_; }

  bool IsDeclaration() const { return block_stmt_ == nullptr; }

  bool IsExecutableBlockStmt() const;

 private:
  std::string name_;
  std::vector<ParameterListItem> param_list_;
  std::unique_ptr<Type> return_type_;
  std::unique_ptr<BlockStmt> block_stmt_;
  // 目前只有lambda用到
  std::vector<CaptureIdItem> capture_ids_;
};

}  // namespace wamon
