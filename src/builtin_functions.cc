#include "wamon/builtin_functions.h"
#include "wamon/ast.h"
#include "wamon/exception.h"
#include "wamon/type_checker.h"

namespace wamon {

static void register_builtin_funcs(std::unordered_map<std::string, std::function<std::shared_ptr<Variable>(std::vector<std::shared_ptr<Variable>>&&)>>& funcs) {
  funcs["len"] = [](std::vector<std::shared_ptr<Variable>>&& params) -> std::shared_ptr<Variable> {
    if (params.size() != 1) {

    }
    if (IsListType(params[0]->GetType())) {

    }
    if (IsStringType(params[0]->GetType())) {
      int len = AsStringVariable(params[0])->GetValue().size();
      return std::make_shared<IntVariable>(len, "");
    }
    return nullptr;
  };
}

BuiltinFunctions::BuiltinFunctions() {
  register_builtin_funcs(builtin_funcs_);
}

std::unique_ptr<Type> BuiltinFunctions::len_type_check(const TypeChecker& tc, FuncCallExpr* call_expr) {
  auto& parameters = call_expr->GetParameters();
  if (parameters.size() != 1) {
    throw WamonExecption("builtinfunction len get invalid params count : {} , need 1", parameters.size());
  }
  auto param_type = tc.GetExpressionType(parameters[0].get());
  if (IsStringType(param_type) || IsListType(param_type)) {
    // type check success
    return TypeFactory<int>::Get();
  }
  throw WamonTypeCheck("builtinfunction len typecheck error", param_type->GetTypeInfo(), "need string or list type");
}

std::unique_ptr<Type> BuiltinFunctions::TypeCheck(const std::string& name, const TypeChecker& tc, FuncCallExpr* call_expr) {
  if (name == "len") {
    return len_type_check(tc, call_expr);
  }
  throw WamonExecption("not implemented now");
}

}