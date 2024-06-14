#include "wamon/builtin_functions.h"

#include <iostream>
#include <string>

#include "nlohmann/json.hpp"
#include "wamon/ast.h"
#include "wamon/exception.h"
#include "wamon/interpreter.h"
#include "wamon/package_unit.h"
#include "wamon/type_checker.h"
#include "wamon/variable.h"

namespace wamon {

static auto _print(Interpreter& ip, std::vector<std::shared_ptr<Variable>>&& params) -> std::shared_ptr<Variable> {
  nlohmann::json j;
  for (auto& each : params) {
    j.push_back(each->Print());
  }
  std::cout << j.dump() << std::endl;
  return std::make_shared<VoidVariable>();
}

static auto _context_stack(Interpreter& ip, std::vector<std::shared_ptr<Variable>>&& params)
    -> std::shared_ptr<Variable> {
  assert(params.empty());
  auto descs = ip.GetContextStack();
  // global and context_stack self at least
  assert(descs.size() >= 2);
  auto it = descs.begin();
  descs.erase(it);
  auto ret = VariableFactory(TypeFactory<std::vector<std::string>>::Get(), Variable::ValueCategory::RValue, "", ip);
  for (auto& each : descs) {
    auto desc_v = VariableFactory(TypeFactory<std::string>::Get(), Variable::ValueCategory::RValue, "", ip);
    AsStringVariable(desc_v)->SetValue(each);
    AsListVariable(ret)->PushBack(std::move(desc_v));
  }
  return ret;
}

static auto _to_string(Interpreter& ip, std::vector<std::shared_ptr<Variable>>&& params) -> std::shared_ptr<Variable> {
  assert(params.size() == 1);
  auto& v = params[0];
  std::string result;
  if (IsIntType(v->GetType())) {
    result = std::to_string(AsIntVariable(v)->GetValue());
  } else if (IsDoubleType(v->GetType())) {
    result = std::to_string(AsDoubleVariable(v)->GetValue());
  } else if (IsBoolType(v->GetType())) {
    result = AsBoolVariable(v)->GetValue() == true ? "true" : "false";
  } else if (IsByteType(v->GetType())) {
    result.push_back(static_cast<char>(AsByteVariable(v)->GetValue()));
  } else {
    auto enum_def = ip.GetPackageUnit().FindEnum(v->GetTypeInfo());
    assert(enum_def != nullptr);
    const auto& enum_item = AsEnumVariable(v)->GetEnumItem();
    result = enum_def->GetEnumName() + ":" + enum_item;
  }
  return std::make_shared<StringVariable>(result, wamon::Variable::ValueCategory::RValue, "");
}

static void register_builtin_handles(const std::string& prefix,
                                     std::unordered_map<std::string, BuiltinFunctions::HandleType>& handles) {
  handles[prefix + "print"] = _print;
  handles[prefix + "to_string"] = _to_string;
  handles[prefix + "context_stack"] = _context_stack;
}

static auto _print_check(const PackageUnit& pu, const std::vector<std::unique_ptr<Type>>& params_type)
    -> std::unique_ptr<Type> {
  return TypeFactory<void>::Get();
}

static auto _context_stack_check(const PackageUnit& pu, const std::vector<std::unique_ptr<Type>>& params_type)
    -> std::unique_ptr<Type> {
  if (params_type.size() != 0) {
    throw WamonException("context_stack type_check error, params_type.size() == {}", params_type.size());
  }
  return TypeFactory<std::vector<std::string>>::Get();
}

static auto _to_string_check(const PackageUnit& pu, const std::vector<std::unique_ptr<Type>>& params_type)
    -> std::unique_ptr<Type> {
  if (params_type.size() != 1) {
    throw WamonException("to_string type_check error ,params_type.size() == {}", params_type.size());
  }
  auto& type = params_type[0];
  if (IsIntType(type) || IsDoubleType(type) || IsBoolType(type) || IsByteType(type)) {
    return TypeFactory<std::string>::Get();
  }
  if (pu.FindEnum(type->GetTypeInfo()) == nullptr) {
    throw WamonException("to_string type_check error, invalid type {}", type->GetTypeInfo());
  } else {
    return TypeFactory<std::string>::Get();
  }
  throw WamonException("to_string type_check error, type {} cant not be to_string", type->GetTypeInfo());
}

static void register_builtin_checks(const std::string& prefix,
                                    std::unordered_map<std::string, BuiltinFunctions::CheckType>& checks) {
  checks[prefix + "print"] = _print_check;
  checks[prefix + "to_string"] = _to_string_check;
  checks[prefix + "context_stack"] = _context_stack_check;
}

BuiltinFunctions::BuiltinFunctions() {}

void BuiltinFunctions::RegisterWamonBuiltinFunction(BuiltinFunctions& obj) {
  register_builtin_checks("wamon$", obj.builtin_checks_);
  register_builtin_handles("wamon$", obj.builtin_handles_);
}

std::unique_ptr<Type> BuiltinFunctions::TypeCheck(const std::string& name, const PackageUnit& pu,
                                                  const std::vector<std::unique_ptr<Type>>& params_type) const {
  auto check = builtin_checks_.find(name);
  if (check == builtin_checks_.end()) {
    throw WamonException("BuiltinFunctions::TypeCheck error, not implemented {} now", name);
  }
  return check->second(pu, params_type);
}

}  // namespace wamon
