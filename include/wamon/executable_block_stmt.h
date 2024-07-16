#pragma once

#include <wamon/interpreter.h>
#include <wamon/variable.h>
#include <wamon/variable_void.h>

#include <functional>

#include "wamon/ast.h"

namespace wamon {

class ExecutableBlockStmt : public BlockStmt {
 public:
  using executable_type = std::function<std::shared_ptr<Variable>(Interpreter&)>;

  ExecutableBlockStmt() = default;

  std::string GetStmtName() override { return "executable_block_stmt"; }

  ExecuteResult Execute(Interpreter& ip) override {
    if (executable_) {
      return ExecuteResult::Return(executable_(ip));
    }
    return ExecuteResult::Return(GetVoidVariable());
  }

  void SetExecutable(executable_type&& e) { executable_ = std::move(e); }

  void SetExecutable(const executable_type& e) { executable_ = e; }

 private:
  executable_type executable_;
};

}  // namespace wamon
