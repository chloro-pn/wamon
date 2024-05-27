#include "wamon/ast.h"

#include "wamon/exception.h"
#include "wamon/inner_type_method.h"
#include "wamon/interpreter.h"

namespace wamon {

std::shared_ptr<Variable> TypeExpr::Calculate(Interpreter&) {
  return std::make_shared<TypeVariable>(GetType()->Clone());
}

std::shared_ptr<Variable> FuncCallExpr::Calculate(Interpreter& interpreter) {
  // callable or function
  std::vector<std::shared_ptr<Variable>> params;
  for (auto& each : parameters_) {
    auto v = each->Calculate(interpreter);
    params.push_back(std::move(v));
  }
  std::string func_name;
  if (type == FuncCallType::FUNC) {
    func_name = dynamic_cast<IdExpr*>(caller_.get())->GenerateIdent();
  } else if (type == FuncCallType::BUILT_IN_FUNC) {
    func_name = dynamic_cast<IdExpr*>(caller_.get())->GetId();
  }
  if (type == FuncCallType::FUNC) {
    auto funcdef = interpreter.GetPackageUnit().FindFunction(func_name);
    auto result = interpreter.CallFunction(funcdef, std::move(params));
    return result;
  } else if (type == FuncCallType::CALLABLE) {
    auto obj = caller_->Calculate(interpreter);
    assert(obj != nullptr);
    auto result = interpreter.CallCallable(obj, std::move(params));
    return result;
  } else if (type == FuncCallType::BUILT_IN_FUNC) {
    auto result = interpreter.CallFunction(func_name, std::move(params));
    return result;
  } else if (type == FuncCallType::OPERATOR_OVERRIDE) {
    auto obj = caller_->Calculate(interpreter);
    assert(obj != nullptr);
    auto method_def = interpreter.GetPackageUnit().FindStruct(obj->GetTypeInfo())->GetMethod(method_name);
    assert(method_def != nullptr);
    auto result = interpreter.CallMethod(obj, method_def, std::move(params));
    return result;
  }
  throw WamonExecption("FuncCallExpr calculate error, invalid type");
}

std::shared_ptr<Variable> MethodCallExpr::Calculate(Interpreter& interpreter) {
  std::vector<std::shared_ptr<Variable>> params;
  for (auto& each : parameters_) {
    auto v = each->Calculate(interpreter);
    params.push_back(std::move(v));
  }
  auto v = caller_->Calculate(interpreter);
  if (!IsStructType(v->GetType())) {
    auto result = interpreter.CallMethod(v, method_name_, std::move(params));
    return result;
  }
  auto structdef = interpreter.GetPackageUnit().FindStruct(v->GetTypeInfo());
  while (structdef->IsTrait() == true) {
    v = AsStructVariable(v)->GetTraitObj();
    structdef = interpreter.GetPackageUnit().FindStruct(v->GetTypeInfo());
  }
  auto methoddef = structdef->GetMethod(method_name_);
  auto result = interpreter.CallMethod(v, methoddef, std::move(params));
  return result;
}

std::shared_ptr<Variable> BinaryExpr::Calculate(Interpreter& interpreter) {
  auto leftop = left_->Calculate(interpreter);
  std::shared_ptr<Variable> rightop;
  // 数据成员访问运算符特殊处理，因为第二个操作数的计算完全依赖于第一个操作数的计算，因此这里我们仅仅将其转换为string的Variable
  // right_的动态类型为IdExpr指针需要类型检测阶段保证
  if (op_ == Token::MEMBER_ACCESS) {
    rightop = std::make_shared<StringVariable>(dynamic_cast<IdExpr*>(right_.get())->GetId(),
                                               Variable::ValueCategory::RValue, "");
  } else {
    rightop = right_->Calculate(interpreter);
  }
  return interpreter.CalculateOperator(op_, leftop, rightop);
}

std::shared_ptr<Variable> UnaryExpr::Calculate(Interpreter& interpreter) {
  auto childop = operand_->Calculate(interpreter);
  // 如果move应用到一个左值，我们通过将其变换为右值然后使用Clone操作构造一个新的值
  // 因为Variable的实现遵循以下逻辑：如果Clone一个右值，则会尽可能复用右值的数据成员构造新的对象
  // 最后调用该左值的DefaultConstruct函数构造其默认状态
  if (op_ == Token::MOVE && childop->GetValueCategory() == Variable::ValueCategory::LValue) {
    childop->ChangeTo(Variable::ValueCategory::RValue);
    auto new_v = childop->Clone();
    childop->DefaultConstruct();
    childop->ChangeTo(Variable::ValueCategory::LValue);
    return new_v;
  }
  auto ret = interpreter.CalculateOperator(op_, childop);
  return ret;
}

// 如果id指向的是变量，则从调用栈中找到该变量返回，如果id指向的是函数，则构造一个右值的FunctionVariable对象返回
std::shared_ptr<Variable> IdExpr::Calculate(Interpreter& interpreter) {
  if (type_ == Type::Invalid) {
    throw WamonExecption("IdExpr {} {} Calculate error, type invalid", package_name_, id_name_);
  }
  if (type_ == Type::Variable || type_ == Type::Callable) {
    return interpreter.FindVariableById(GenerateIdent());
  } else if (type_ == Type::BuiltinFunc) {
    auto type = interpreter.GetPackageUnit().GetBuiltinFunctions().GetType(GetId());
    assert(type != nullptr);
    auto ret = std::make_unique<FunctionVariable>(GetParamType(type), GetReturnType(type),
                                                  Variable::ValueCategory::RValue, "");
    ret->SetFuncName(GetId());
    return ret;
  } else {
    auto func_def = interpreter.GetPackageUnit().FindFunction(GenerateIdent());
    auto type = func_def->GetType();
    auto ret = std::make_shared<FunctionVariable>(GetParamType(type), GetReturnType(type),
                                                  Variable::ValueCategory::RValue, "");
    ret->SetFuncName(GenerateIdent());
    return ret;
  }
}

std::shared_ptr<Variable> SelfExpr::Calculate(Interpreter& interpreter) { return interpreter.GetSelfObject(); }

std::shared_ptr<Variable> LambdaExpr::Calculate(Interpreter& interpreter) {
  auto func_def = interpreter.GetPackageUnit().FindFunction(lambda_func_name_);
  std::vector<std::shared_ptr<Variable>> capture_variables;
  auto& ids = func_def->GetCaptureIds();
  for (auto& each : ids) {
    auto v = interpreter.FindVariableById(each.id);
    if (!v->IsRValue()) {
      v = v->Clone();
      v->SetName(each.id);
    }
    capture_variables.push_back(v);
  }
  auto call_obj =
      VariableFactory(func_def->GetType(), Variable::ValueCategory::RValue, "", interpreter.GetPackageUnit());
  AsFunctionVariable(call_obj.get())->SetFuncName(lambda_func_name_);
  AsFunctionVariable(call_obj.get())->SetCaptureVariables(std::move(capture_variables));
  return call_obj;
}

std::shared_ptr<Variable> NewExpr::Calculate(Interpreter& interpreter) {
  auto v = VariableFactory(type_, Variable::ValueCategory::RValue, "", interpreter.GetPackageUnit());
  std::vector<std::shared_ptr<Variable>> fields;
  for (auto& each : parameters_) {
    fields.push_back(each->Calculate(interpreter));
  }
  if (fields.empty()) {
    v->DefaultConstruct();
  } else {
    v->ConstructByFields(std::move(fields));
  }
  return v;
}

ExecuteResult BlockStmt::Execute(Interpreter& interpreter) {
  interpreter.EnterContext<RuntimeContextType::Block>();
  for (auto& each : block_) {
    ExecuteResult step = each->Execute(interpreter);
    if (step.state_ != ExecuteState::Next) {
      interpreter.LeaveContext();
      return step;
    }
  }
  interpreter.LeaveContext();
  return ExecuteResult::Next();
}

ExecuteResult ForStmt::Execute(Interpreter& interpreter) {
  interpreter.EnterContext<RuntimeContextType::FOR>();
  init_->Calculate(interpreter);
  while (true) {
    auto v = check_->Calculate(interpreter);
    bool check = AsBoolVariable(v)->GetValue();
    if (check == false) {
      break;
    }
    ExecuteResult er = block_->Execute(interpreter);
    if (er.state_ == ExecuteState::Continue || er.state_ == ExecuteState::Next) {
      update_->Calculate(interpreter);
      continue;
    } else if (er.state_ == ExecuteState::Break) {
      interpreter.LeaveContext();
      return ExecuteResult::Next();
    }
    interpreter.LeaveContext();
    // Result
    return er;
  }
  interpreter.LeaveContext();
  return ExecuteResult::Next();
}

ExecuteResult IfStmt::Execute(Interpreter& interpreter) {
  bool need_leave_context = false;
  auto v = check_->Calculate(interpreter);
  bool check = AsBoolVariable(v)->GetValue();
  ExecuteResult er = ExecuteResult::Next();
  if (check == true) {
    interpreter.EnterContext<RuntimeContextType::IF>();
    need_leave_context = true;
    er = if_block_->Execute(interpreter);
  } else {
    if (else_block_ != nullptr) {
      interpreter.EnterContext<RuntimeContextType::ELSE>();
      need_leave_context = true;
      er = else_block_->Execute(interpreter);
    } else {
      er = ExecuteResult::Next();
    }
  }
  if (need_leave_context == true) {
    interpreter.LeaveContext();
  }
  if (er.state_ == ExecuteState::Next || er.state_ == ExecuteState::Break) {
    er.state_ = ExecuteState::Next;
  }
  // 如果返回continue或者return的话，需要上层调用栈处理
  return er;
}

ExecuteResult WhileStmt::Execute(Interpreter& interpreter) {
  interpreter.EnterContext<RuntimeContextType::WHILE>();
  while (true) {
    auto v = check_->Calculate(interpreter);
    bool check = AsBoolVariable(v)->GetValue();
    if (check == false) {
      interpreter.LeaveContext();
      return ExecuteResult::Next();
    }
    ExecuteResult er = block_->Execute(interpreter);
    if (er.state_ == ExecuteState::Continue || er.state_ == ExecuteState::Next) {
      continue;
    } else if (er.state_ == ExecuteState::Break) {
      break;
    } else {
      interpreter.LeaveContext();
      return er;
    }
  }
  interpreter.LeaveContext();
  return ExecuteResult::Next();
}

ExecuteResult ReturnStmt::Execute(Interpreter& interpreter) {
  if (return_ != nullptr) {
    auto v = return_->Calculate(interpreter);
    return ExecuteResult::Return(v->IsRValue() ? std::move(v) : v->Clone());
  }
  // 如果是return;语句则填充一个void类型返回，简化后续处理
  return ExecuteResult::Return(std::make_shared<VoidVariable>());
}

// 对于表达式语句而言，我们只是计算其临时变量，在下一个语句开始执行前就会销毁该变量
// 计算可能有副作用，因此不能优化掉这里
ExecuteResult ExpressionStmt::Execute(Interpreter& interpreter) {
  auto v = expr_->Calculate(interpreter);
  return ExecuteResult::Next();
}

template <typename T>
std::shared_ptr<T> ptr_cast(std::unique_ptr<T>&& ptr) {
  return std::shared_ptr<T>(std::move(ptr));
}

template <typename T>
std::unique_ptr<T> ptr_cast(std::shared_ptr<T>&& ptr) {
  assert(ptr.use_count() == 1);
  std::unique_ptr<T> ret(ptr.get());
  ptr.reset();
  return ret;
}

ExecuteResult VariableDefineStmt::Execute(Interpreter& interpreter) {
  auto context = interpreter.GetCurrentContext();
  auto v = VariableFactoryShared(type_, Variable::ValueCategory::LValue, var_name_, interpreter.GetPackageUnit());
  std::vector<std::shared_ptr<Variable>> fields;
  for (auto& each : constructors_) {
    fields.push_back(each->Calculate(interpreter));
  }
  if (fields.empty() == true) {
    assert(IsRef() == false);
    v->DefaultConstruct();
  } else if (fields.size() == 1 && fields[0]->GetTypeInfo() == v->GetTypeInfo()) {
    if (IsRef()) {
      // if rvalue
      if (fields[0]->IsRValue()) {
        throw WamonExecption("VariableDefineStmt::Execute failed, let ref can not be constructed by rvalue");
      } else {
        v = fields[0];
      }
    } else {
      v = fields[0]->IsRValue() ? fields[0] : fields[0]->Clone();
    }
    v->SetName(var_name_);
    v->ChangeTo(Variable::ValueCategory::LValue);
  } else {
    assert(IsRef() == false);
    v->ConstructByFields(fields);
  }
  context->RegisterVariable(std::move(v));
  return ExecuteResult::Next();
}

}  // namespace wamon
