#include "wamon/ast.h"

#include "wamon/inner_type_method.h"
#include "wamon/interpreter.h"

namespace wamon {

std::shared_ptr<Variable> FuncCallExpr::Calculate(Interpreter& interpreter) {
  // callable or function
  std::vector<std::shared_ptr<Variable>> params;
  for (auto& each : parameters_) {
    auto v = each->Calculate(interpreter);
    params.push_back(std::move(v));
  }
  if (type == FuncCallType::FUNC) {
    auto funcdef = interpreter.GetPackageUnit().FindFunction(func_name_);
    interpreter.EnterContext<RuntimeContextType::Function>();
    auto result = interpreter.CallFunction(funcdef, std::move(params));
    interpreter.LeaveContext();
    return result;
  } else if (type == FuncCallType::CALLABLE) {
    auto obj = interpreter.FindVariableById<false>(func_name_);
    assert(obj != nullptr);
    interpreter.EnterContext<RuntimeContextType::Callable>();
    auto result = interpreter.CallCallable(obj, std::move(params));
    interpreter.LeaveContext();
    return result;
  } else if (type == FuncCallType::BUILT_IN_FUNC) {
    interpreter.EnterContext<RuntimeContextType::Function>();
    auto result = interpreter.CallFunction(func_name_, std::move(params));
    interpreter.LeaveContext();
    return result;
  } else if (type == FuncCallType::OPERATOR_OVERRIDE) {
    auto obj = interpreter.FindVariableById<false>(func_name_);
    assert(obj != nullptr);
    auto method_def = interpreter.GetPackageUnit().FindStruct(obj->GetTypeInfo())->GetMethod(method_name);
    assert(method_def != nullptr);
    interpreter.EnterContext<RuntimeContextType::Method>();
    auto result = interpreter.CallMethod(obj, method_def, std::move(params));
    interpreter.LeaveContext();
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
  auto v = interpreter.FindVariableById(id_name_);
  if (IsInnerType(v->GetType())) {
    interpreter.EnterContext<RuntimeContextType::Method>();
    auto result = interpreter.CallMethod(v, method_name_, std::move(params));
    interpreter.LeaveContext();
    return result;
  }
  auto methoddef = interpreter.GetPackageUnit().FindTypeMethod(v->GetTypeInfo(), method_name_);
  interpreter.EnterContext<RuntimeContextType::Method>();
  auto result = interpreter.CallMethod(v, methoddef, std::move(params));
  interpreter.LeaveContext();
  return result;
}

std::shared_ptr<Variable> BinaryExpr::Calculate(Interpreter& interpreter) {
  auto leftop = left_->Calculate(interpreter);
  std::shared_ptr<Variable> rightop;
  // 数据成员访问运算符特殊处理，因为第二个操作数的计算完全依赖于第一个操作数的计算，因此这里我们仅仅将其转换为string的Variable
  // right_的动态类型为IdExpr指针需要类型检测阶段保证
  if (op_ == Token::MEMBER_ACCESS) {
    rightop = std::make_shared<StringVariable>(dynamic_cast<IdExpr*>(right_.get())->GetId(), "");
  } else {
    rightop = right_->Calculate(interpreter);
  }
  return interpreter.CalculateOperator(op_, leftop, rightop);
}

std::shared_ptr<Variable> UnaryExpr::Calculate(Interpreter& interpreter) {
  auto childop = operand_->Calculate(interpreter);
  return interpreter.CalculateOperator(op_, childop);
}

std::shared_ptr<Variable> IdExpr::Calculate(Interpreter& interpreter) {
  if (type_ == Type::Variable) {
    return interpreter.FindVariableById(id_name_);
  } else {
    auto func_def = interpreter.GetPackageUnit().FindFunction(id_name_);
    auto type = func_def->GetType();
    auto ret = std::make_shared<FunctionVariable>(GetParamType(type), GetReturnType(type), "");
    ret->SetFuncName(id_name_);
    return ret;
  }
}

std::shared_ptr<Variable> SelfExpr::Calculate(Interpreter& interpreter) { return interpreter.GetSelfObject(); }

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
    return ExecuteResult::Return(std::move(v));
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

ExecuteResult VariableDefineStmt::Execute(Interpreter& interpreter) {
  auto& context = interpreter.GetCurrentContext();
  auto v = VariableFactory(type_->Clone(), var_name_, interpreter);
  std::vector<std::shared_ptr<Variable>> fields;
  for (auto& each : constructors_) {
    fields.push_back(each->Calculate(interpreter));
  }
  if (fields.empty() == true) {
    v->DefaultConstruct();
  } else {
    v->ConstructByFields(fields);
  }
  context.RegisterVariable(std::move(v));
  return ExecuteResult::Next();
}

}  // namespace wamon
