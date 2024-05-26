#include "wamon/interpreter.h"

#include "wamon/exception.h"
#include "wamon/move_wrapper.h"
#include "wamon/operator.h"
#include "wamon/type.h"

namespace wamon {

Interpreter::Interpreter(PackageUnit& pu, Tag tag) : tag_(tag), pu_(pu) {
  package_context_.type_ = RuntimeContextType::Global;
  if (tag_ == Tag::Default) {
    // 将packge unit中的包变量进行求解并插入包符号表中
    const auto& vars = pu_.GetGlobalVariDefStmt();
    for (const auto& each : vars) {
      each->Execute(*this);
    }
  }
}

void Interpreter::ExecGlobalVariDefStmt() {
  if (tag_ == Tag::Default) {
    throw WamonExecption("Interpreter::ExecGlobalVariDefStmt should not be called under Default Tag");
  }
  // 将packge unit中的包变量进行求解并插入包符号表中
  const auto& vars = pu_.GetGlobalVariDefStmt();
  for (const auto& each : vars) {
    each->Execute(*this);
  }
}

// params分类两类，一类是函数调用过程中传入的参数，一类是lambda表达式捕获到的变量
std::shared_ptr<Variable> Interpreter::CallFunction(const FunctionDef* function_def,
                                                    std::vector<std::shared_ptr<Variable>>&& params) {
  if (function_def == nullptr) {
    throw WamonExecption("Interpreter::CallFunction error, function == nullptr");
  }
  EnterContext<RuntimeContextType::Function>();
  auto param_name = function_def->GetParamList().begin();
  auto capture_name = function_def->GetCaptureIds().begin();
  for (auto param : params) {
    if (param_name != function_def->GetParamList().end()) {
      assert(capture_name == function_def->GetCaptureIds().begin());
      if (param_name->is_ref == true) {
        if (param->IsRValue()) {
          throw WamonExecption("Interpreter::CallFunction error, ref parameter can not be rvalue");
        }
        GetCurrentContext()->RegisterVariable(param, param_name->name);
      } else {
        GetCurrentContext()->RegisterVariable(param->IsRValue() ? param : param->Clone(), param_name->name);
      }
      ++param_name;
    } else {
      // 函数调用传入的参数总是在前面，捕获的变量跟在后面，因此当注册捕获变量时，参数应该已经注册完毕
      assert(param_name == function_def->GetParamList().end());
      GetCurrentContext()->RegisterVariable(param->IsRValue() ? param : param->Clone(), *capture_name);
      ++capture_name;
    }
  }
  auto result = function_def->block_stmt_->Execute(*this);
  if (result.state_ != ExecuteState::Return) {
    throw WamonExecption("interpreter call function {} error, diden't end by return stmt",
                         function_def->GetFunctionName());
  }
  LeaveContext();
  return result.result_;
}

std::shared_ptr<Variable> Interpreter::CallCallable(std::shared_ptr<Variable> callable,
                                                    std::vector<std::shared_ptr<Variable>>&& params) {
  auto& func_name = AsFunctionVariable(callable)->GetFuncName();
  auto obj = AsFunctionVariable(callable)->GetObj();
  auto& capture_variable = AsFunctionVariable(callable)->GetCaptureVariables();
  if (func_name.empty() && obj == nullptr) {
    throw WamonExecption("Interpreter.CallCallable error, the callable is null state, cant not be called");
  }
  for (auto& each : capture_variable) {
    if (each->IsRValue()) {
      params.push_back(std::move(each));
    } else {
      params.push_back(each->Clone());
    }
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
    throw WamonExecption("Interpreter.CallMethod error, method {} is declaration", method_def->GetMethodName());
  }
  EnterContext<RuntimeContextType::Method>();
  auto param_name = method_def->GetParamList().begin();
  for (auto param : params) {
    assert(param_name != method_def->GetParamList().end());
    if (param_name->is_ref == true) {
      if (param->IsRValue() == true) {
        throw WamonExecption("Interpreter::CallMethod error, ref parameter can not be rvalue");
      }
      GetCurrentContext()->RegisterVariable(param, param_name->name);
    } else {
      GetCurrentContext()->RegisterVariable(param->IsRValue() ? param : param->Clone(), param_name->name);
    }
    ++param_name;
  }
  GetCurrentContext()->RegisterVariable(obj, "__self__");
  auto result = method_def->GetBlockStmt()->Execute(*this);
  if (result.state_ != ExecuteState::Return) {
    throw WamonExecption("interpreter call method {}.{} error, diden't end by return stmt", obj->GetName(),
                         method_def->GetMethodName());
  }
  LeaveContext();
  return result.result_->IsRValue() ? result.result_ : result.result_->Clone();
}

void Interpreter::RegisterCppFunctions(const std::string& name, std::unique_ptr<Type> func_type,
                                       BuiltinFunctions::HandleType ht) {
  if (IsFuncType(func_type) == false) {
    throw WamonExecption("RegisterCppFunctions error, {} have non-function type : {}", name, func_type->GetTypeInfo());
  }
  auto ftype = func_type->Clone();
  MoveWrapper<decltype(ftype)> mw(std::move(ftype));
  auto check_f = [name = name,
                  mw = mw](const std::vector<std::unique_ptr<Type>>& params_type) mutable -> std::unique_ptr<Type> {
    auto func_type = std::move(mw).Get();
    auto par_type = GetParamType(func_type);
    size_t param_size = par_type.size();
    if (param_size != params_type.size()) {
      throw WamonExecption("RegisterCppFunctions error, func {} params type count {} != {}", name, params_type.size(),
                           param_size);
    }
    for (size_t i = 0; i < param_size; ++i) {
      if (!IsSameType(par_type[i], params_type[i])) {
        throw WamonExecption("RegisterCppFunctions error, func {} {}th param type mismatch : {} != {}", name, i,
                             par_type[i]->GetTypeInfo(), params_type[i]->GetTypeInfo());
      }
    }
    return GetReturnType(func_type);
  };
  RegisterCppFunctions(name, std::move(check_f), std::move(ht));
  GetPackageUnit().GetBuiltinFunctions().SetTypeForFunction(name, std::move(func_type));
}

}  // namespace wamon
