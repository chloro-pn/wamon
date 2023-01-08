#include "wamon/type.h"
#include "wamon/ast.h"

namespace wamon {

void ArrayType::SetCount(std::unique_ptr<Expression> count) {
  count_expr_ = std::move(count);
}

}