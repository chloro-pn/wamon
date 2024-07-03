#include "wamon/variable_void.h"

namespace wamon {

std::shared_ptr<Variable> GetVoidVariable() { return std::make_shared<VoidVariable>(); }

}  // namespace wamon
