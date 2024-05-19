#include "wamon/builtin_functions.h"

#include "wamon/ast.h"
#include "wamon/exception.h"
#include "wamon/output.h"
#include "wamon/type_checker.h"

namespace wamon {

static auto _print(std::vector<std::shared_ptr<Variable>>&& params) -> std::shared_ptr<Variable> {
  StdOutput output;
  for (auto& each : params) {
    each->Print(output);
    output.OutPutString(" ");
  }
  return std::make_shared<VoidVariable>();
}

static void register_builtin_handles(std::unordered_map<std::string, BuiltinFunctions::HandleType>& handles) {
  handles["print"] = _print;
}

static auto _print_check(const std::vector<std::unique_ptr<Type>>& params_type) -> std::unique_ptr<Type> {
  return TypeFactory<void>::Get();
}

static void register_builtin_checks(std::unordered_map<std::string, BuiltinFunctions::CheckType>& checks) {
  checks["print"] = _print_check;
}

BuiltinFunctions::BuiltinFunctions() {
  register_builtin_checks(builtin_checks_);
  register_builtin_handles(builtin_handles_);
}

std::unique_ptr<Type> BuiltinFunctions::TypeCheck(const std::string& name,
                                                  const std::vector<std::unique_ptr<Type>>& params_type) {
  auto check = builtin_checks_.find(name);
  if (check == builtin_checks_.end()) {
    throw WamonExecption("BuiltinFunctions::TypeCheck error, not implemented {} now", name);
  }
  return check->second(params_type);
}

}  // namespace wamon
