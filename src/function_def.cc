#include "wamon/function_def.h"

#include "wamon/ast.h"
#include "wamon/executable_block_stmt.h"

namespace wamon {

void FunctionDef::SetBlockStmt(std::unique_ptr<BlockStmt>&& block_stmt) { block_stmt_ = std::move(block_stmt); }

bool FunctionDef::IsExecutableBlockStmt() const {
  return dynamic_cast<ExecutableBlockStmt*>(block_stmt_.get()) != nullptr;
}

}  // namespace wamon
