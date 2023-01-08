#pragma once

#include <string>
#include <vector>
#include <memory>
#include <utility>

#include "wamon/type.h"
#include "wamon/ast.h"

namespace wamon {

class FunctionDef {
 public:
  explicit FunctionDef(const std::string& name) : name_(name) {

  }

  const std::string& GetFunctionName() const {
    return name_;
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