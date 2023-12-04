#include "wamon/function_def.h"

#include "wamon/ast.h"

namespace wamon {

void FunctionDef::SetBlockStmt(std::unique_ptr<BlockStmt>&& block_stmt) { block_stmt_ = std::move(block_stmt); }

}  // namespace wamon
