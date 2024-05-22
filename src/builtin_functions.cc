#include "wamon/builtin_functions.h"

#include <string>

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

static auto _to_string(std::vector<std::shared_ptr<Variable>>&& params) -> std::shared_ptr<Variable> {
  assert(params.size() == 1);
  auto& v = params[0];
  std::string result;
  if (IsIntType(v->GetType())) {
    result = std::to_string(AsIntVariable(v)->GetValue());
  } else if (IsDoubleType(v->GetType())) {
    result = std::to_string(AsDoubleVariable(v)->GetValue());
  } else if (IsBoolType(v->GetType())) {
    result = AsBoolVariable(v)->GetValue() == true ? "true" : "false";
  } else {
    result.push_back(static_cast<char>(AsByteVariable(v)->GetValue()));
  }
  return std::make_shared<StringVariable>(result, wamon::Variable::ValueCategory::RValue, "");
}

static void register_builtin_handles(std::unordered_map<std::string, BuiltinFunctions::HandleType>& handles) {
  handles["print"] = _print;
  handles["to_string"] = _to_string;
}

static auto _print_check(const std::vector<std::unique_ptr<Type>>& params_type) -> std::unique_ptr<Type> {
  return TypeFactory<void>::Get();
}

static auto _to_string_check(const std::vector<std::unique_ptr<Type>>& params_type) -> std::unique_ptr<Type> {
  if (params_type.size() != 1) {
    throw WamonExecption("to_string type_check error ,params_type.size() == {}", params_type.size());
  }
  auto& type = params_type[0];
  if (IsIntType(type) || IsDoubleType(type) || IsBoolType(type) || IsByteType(type)) {
    return TypeFactory<std::string>::Get();
  }
  throw WamonExecption("to_string type_check error, type {} cant not be to_string", type->GetTypeInfo());
}

static void register_builtin_checks(std::unordered_map<std::string, BuiltinFunctions::CheckType>& checks) {
  checks["print"] = _print_check;
  checks["to_string"] = _to_string_check;
}

BuiltinFunctions::BuiltinFunctions() {
  register_builtin_checks(builtin_checks_);
  register_builtin_handles(builtin_handles_);
}

std::unique_ptr<Type> BuiltinFunctions::TypeCheck(const std::string& name,
                                                  const std::vector<std::unique_ptr<Type>>& params_type) const {
  auto check = builtin_checks_.find(name);
  if (check == builtin_checks_.end()) {
    throw WamonExecption("BuiltinFunctions::TypeCheck error, not implemented {} now", name);
  }
  return check->second(params_type);
}

}  // namespace wamon
