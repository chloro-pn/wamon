#include "wamon/interpreter.h"
#include "wamon/operator.h"

namespace wamon {

Interpreter::Interpreter(const PackageUnit& pu) : pu_(pu) {
  package_context_.type_ = RuntimeContextType::Global;
  // 将packge unit中的包变量进行求解并插入包符号表中
  const auto& vars = pu_.GetGlobalVariDefStmt();
  for(const auto& each : vars) {
    each->Execute(*this);
  }
}

std::shared_ptr<Variable> Interpreter::CalculateOperator(Token op, const std::shared_ptr<Variable>& left, const std::shared_ptr<Variable>& right) {
  return Operator::Instance().Calculate(op, left, right);
}

std::shared_ptr<Variable> Interpreter::CalculateOperator(Token op, const std::shared_ptr<Variable>& operand) {
  return Operator::Instance().Calculate(op, operand);
}

std::shared_ptr<Variable> Interpreter::CallFunction(const FunctionDef* function_def, std::vector<std::shared_ptr<Variable>>&& params) {
  auto param_name = function_def->GetParamList().begin();
  for(auto param : params) {
    assert(param_name != function_def->GetParamList().end());
    GetCurrentContext().RegisterVariable(param->Clone(), param_name->second);
    ++param_name;
  }
  auto result = function_def->block_stmt_->Execute(*this);
  if (result.state_ != ExecuteState::Return) {
    throw WamonExecption("interpreter call function {} error, diden't end by return stmt", function_def->GetFunctionName());
  }
  return result.result_;
}

std::shared_ptr<Variable> Interpreter::CallCallable(std::shared_ptr<Variable> callable, std::vector<std::shared_ptr<Variable>>&& params) {
  auto& func_name = AsFunctionVariable(callable)->GetFuncName();
  auto obj = AsFunctionVariable(callable)->GetObj();
  if (obj != nullptr) {
    auto method_name = OperatorDef::CreateName(Token::LEFT_PARENTHESIS, params);
    auto method_def = GetPackageUnit().FindStruct(obj->GetTypeInfo())->GetMethod(method_name);
    assert(method_def != nullptr);
    EnterContext<RuntimeContextType::Method>();
    auto result = CallMethod(obj, method_def, std::move(params));
    LeaveContext();
    return result;
  } else {
    return CallFunctionByName(func_name, std::move(params));
  }
}

std::shared_ptr<Variable> Interpreter::CallMethod(std::shared_ptr<Variable> obj, const MethodDef* method_def, std::vector<std::shared_ptr<Variable>>&& params) {
  auto param_name = method_def->GetParamList().begin();
  for(auto param : params) {
    assert(param_name != method_def->GetParamList().end());
    GetCurrentContext().RegisterVariable(param->Clone(), param_name->second);
    ++param_name;
  }
  GetCurrentContext().RegisterVariable(obj, "__self__");
  auto result = method_def->GetBlockStmt()->Execute(*this);
  if (result.state_ != ExecuteState::Return) {
    throw WamonExecption("interpreter call method {}.{} error, diden't end by return stmt", obj->GetName(), method_def->GetMethodName());
  }
  return result.result_;
}

}