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

  void AddParamList(const std::string& type, const std::string& var) {
    param_list_.push_back({type, var});
  }

  void SetReturnType(const std::string& type) {
    return_type_ = type;
  }

  void SetBlockStmt(std::unique_ptr<BlockStmt>&& block_stmt) {
    block_stmt_ = std::move(block_stmt);
  }

 private:
  std::string name_;
  std::vector<std::pair<std::string, std::string>> param_list_;
  std::string return_type_;
  std::unique_ptr<BlockStmt> block_stmt_;
};

}