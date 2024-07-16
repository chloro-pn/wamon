#include "wamon/interpreter.h"

#include "wamon/exception.h"
#include "wamon/operator.h"
#include "wamon/parser.h"
#include "wamon/parsing_package.h"
#include "wamon/scanner.h"
#include "wamon/type.h"
#include "wamon/type_checker.h"

namespace wamon {

Interpreter::Interpreter(PackageUnit& pu) : pu_(pu) {
  package_context_.type_ = RuntimeContextType::Global;
  package_context_.desc_ = "global";
  // 将packge unit中的包变量进行求解并插入包符号表中
  const auto& vars = pu_.GetGlobalVariDefStmt();
  for (const auto& each : vars) {
    each->Execute(*this);
  }
}

std::shared_ptr<Variable> Interpreter::Alloc(const std::unique_ptr<Type>& type,
                                             std::vector<std::shared_ptr<Variable>>&& params) {
  auto v = VariableFactory(type, wamon::Variable::ValueCategory::LValue, "", *this);
  if (params.empty()) {
    v->DefaultConstruct();
  } else {
    v->ConstructByFields(params);
  }
  assert(heap_.find(v) == heap_.end());
  heap_.insert(v);
  std::unique_ptr<Type> ptr_type = std::unique_ptr<PointerType>(new PointerType(type->Clone()));
  auto ptr = VariableFactory(ptr_type, wamon::Variable::ValueCategory::RValue, "", *this);
  AsPointerVariable(ptr)->SetHoldVariable(v);
  return ptr;
}

void Interpreter::Dealloc(std::shared_ptr<Variable> v) {
  auto ptr_to = AsPointerVariable(v)->GetHoldVariable();
  auto it = heap_.find(ptr_to);
  if (it == heap_.end()) {
    throw WamonException("Interpreter::Dealloc error, variable {}:{} not exist in heap", ptr_to->GetName(),
                         ptr_to->GetTypeInfo());
  }
  heap_.erase(it);
}

// params分类两类，一类是函数调用过程中传入的参数，一类是lambda表达式捕获到的变量
std::shared_ptr<Variable> Interpreter::CallFunction(const FunctionDef* function_def,
                                                    std::vector<std::shared_ptr<Variable>>&& params) {
  assert(function_def != nullptr);
  EnterContext<RuntimeContextType::Function>(function_def->GetFunctionName());
  auto param_name = function_def->GetParamList().begin();
  auto capture_name = function_def->GetCaptureIds().begin();
  for (auto param : params) {
    if (param_name != function_def->GetParamList().end()) {
      assert(capture_name == function_def->GetCaptureIds().begin());
      if (param_name->is_ref == true) {
        if (param->IsRValue()) {
          throw WamonException("Interpreter::CallFunction error, ref parameter {} can not be rvalue", param_name->name);
        }
        GetCurrentContext()->RegisterVariable(param, param_name->name);
      } else {
        GetCurrentContext()->RegisterVariable(VariableMoveOrCopy(param), param_name->name);
      }
      ++param_name;
    } else {
      // 函数调用传入的参数总是在前面，捕获的变量跟在后面，因此当注册捕获变量时，参数应该已经注册完毕
      // 捕获变量的值型别已经在lambda表达式中被处理
      assert(param_name == function_def->GetParamList().end());
      GetCurrentContext()->RegisterVariable(param, capture_name->id);
      ++capture_name;
    }
  }
  auto result = function_def->block_stmt_->Execute(*this);
  if (!IsVoidType(function_def->GetReturnType()) && result.state_ != ExecuteState::Return) {
    throw WamonException("interpreter call function {} error, diden't end by return stmt",
                         function_def->GetFunctionName());
  }
  if (IsVoidType(function_def->GetReturnType()) && result.result_ == nullptr) {
    result.result_ = GetVoidVariable();
  }
  LeaveContext();
  return result.result_->IsRValue() ? result.result_ : result.result_->Clone();
}

std::shared_ptr<Variable> Interpreter::CallCallable(std::shared_ptr<Variable> callable,
                                                    std::vector<std::shared_ptr<Variable>>&& params) {
  auto& func_name = AsFunctionVariable(callable)->GetFuncName();
  auto obj = AsFunctionVariable(callable)->GetObj();
  auto& capture_variable = AsFunctionVariable(callable)->GetCaptureVariables();
  if (func_name.empty() && obj == nullptr) {
    throw WamonException("Interpreter.CallCallable error, the callable is null state, cant not be called");
  }
  for (auto& each : capture_variable) {
    params.push_back(each.v);
  }
  if (obj != nullptr) {
    auto method_name = OperatorDef::CreateName(Token::LEFT_PARENTHESIS, params);
    auto method_def = GetPackageUnit().FindStruct(obj->GetTypeInfo())->GetMethod(method_name);
    assert(method_def != nullptr);
    auto result = CallMethod(obj, method_def, std::move(params));
    return result;
  } else {
    return CallFunctionByName(func_name, std::move(params));
  }
}

std::shared_ptr<Variable> Interpreter::CallMethod(std::shared_ptr<Variable> obj, const MethodDef* method_def,
                                                  std::vector<std::shared_ptr<Variable>>&& params) {
  if (method_def->IsDeclaration()) {
    throw WamonException("Interpreter.CallMethod error, method {} is declaration", method_def->GetMethodName());
  }
  EnterContext<RuntimeContextType::Method>(obj->GetTypeInfo() + "::" + method_def->GetMethodName());
  auto param_name = method_def->GetParamList().begin();
  for (auto param : params) {
    assert(param_name != method_def->GetParamList().end());
    if (param_name->is_ref == true) {
      if (param->IsRValue() == true) {
        throw WamonException("Interpreter::CallMethod error, ref parameter can not be rvalue");
      }
      GetCurrentContext()->RegisterVariable(param, param_name->name);
    } else {
      GetCurrentContext()->RegisterVariable(VariableMoveOrCopy(param), param_name->name);
    }
    ++param_name;
  }
  // self比较特殊，和函数参数不同的点在于，它可以作为RValue注册到上下文中，比如：
  // call move v:getAge();
  GetCurrentContext()->RegisterVariable<false>(obj, "__self__");
  auto result = method_def->GetBlockStmt()->Execute(*this);
  if (!IsVoidType(method_def->GetReturnType()) && result.state_ != ExecuteState::Return) {
    throw WamonException("interpreter call method {}.{} error, diden't end by return stmt", obj->GetName(),
                         method_def->GetMethodName());
  }
  if (IsVoidType(method_def->GetReturnType()) && result.result_ == nullptr) {
    result.result_ = GetVoidVariable();
  }
  LeaveContext();
  return result.result_->IsRValue() ? result.result_ : result.result_->Clone();
}

std::shared_ptr<Variable> Interpreter::ExecExpression(TypeChecker& tc, const std::string& package_name,
                                                      const std::string& script) {
  // 恢复parse上下文以正常处理符号定位问题
  tc.GetStaticAnalyzer().GetPackageUnit().SetCurrentParsingPackage(package_name);
  Scanner scan;
  auto tokens = scan.Scan(script);
  auto expr = ParseExpression(pu_, tokens, 0, tokens.size() - 1);
  tc.CheckExpression(expr.get());
  return expr->Calculate(*this);
}

}  // namespace wamon
