#include "wamon/ast.h"
#include "wamon/interpreter.h"

namespace wamon {

std::shared_ptr<Variable> FuncCallExpr::Calcualte(Interpreter& interpreter) {
  // callable or function
  std::vector<std::shared_ptr<Variable>> params;
  for(auto& each : parameters_) {
    auto v = each->Calcualte(interpreter);
    params.push_back(std::move(v));
  }
  auto obj = interpreter.FindVariableById<false>(func_name_);
  if (obj == nullptr) {
    auto funcdef = interpreter.GetPackageUnit().FindFunction(func_name_);
    interpreter.EnterContext<RuntimeContextType::Function>();
    auto result = interpreter.CallFunction(funcdef, std::move(params));
    interpreter.LeaveContext();
    return result;
  } else {
    interpreter.EnterContext<RuntimeContextType::Callable>();
    auto result = interpreter.CallCallable(obj, std::move(params));
    interpreter.LeaveContext();
    return result;
  }
}

std::shared_ptr<Variable> MethodCallExpr::Calcualte(Interpreter& interpreter) {
  std::vector<std::shared_ptr<Variable>> params;
  for(auto& each : parameters_) {
    auto v = each->Calcualte(interpreter);
    params.push_back(std::move(v));
  }
  auto v = interpreter.FindVariableById(id_name_);
  auto methoddef = interpreter.GetPackageUnit().FindTypeMethod(v->GetTypeInfo(), method_name_);
  interpreter.EnterContext<RuntimeContextType::Method>();
  auto result = interpreter.CallMethod(v, methoddef, std::move(params));
  interpreter.LeaveContext();
  return result;
}

std::shared_ptr<Variable> BinaryExpr::Calcualte(Interpreter& interpreter) {
  auto leftop = left_->Calcualte(interpreter);
  auto rightop = right_->Calcualte(interpreter);
  return interpreter.CalculateOperator(op_, leftop, rightop);
}

std::shared_ptr<Variable> UnaryExpr::Calcualte(Interpreter& interpreter) {
  auto childop = operand_->Calcualte(interpreter);
  return interpreter.CalculateOperator(op_, childop);
}

std::shared_ptr<Variable> IdExpr::Calcualte(Interpreter& interpreter) {
  return interpreter.FindVariableById(id_name_);
}

std::shared_ptr<Variable> SelfExpr::Calcualte(Interpreter& interpreter) {
  return interpreter.GetSelfObject();
}

ExecuteResult BlockStmt::Execute(Interpreter& interpreter) {
  interpreter.EnterContext<RuntimeContextType::Block>();
  for(auto& each : block_) {
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
  init_->Calcualte(interpreter);
  while (true) {
    auto v = check_->Calcualte(interpreter);
    bool check = AsBoolVariable(v)->GetValue();
    if (check == false) {
      break;
    }
    ExecuteResult er = block_->Execute(interpreter);
    if (er.state_ == ExecuteState::Continue || er.state_ == ExecuteState::Next) {
      update_->Calcualte(interpreter);
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
  auto v = check_->Calcualte(interpreter);
  bool check = AsBoolVariable(v)->GetValue();
  ExecuteResult er = ExecuteResult::Next();
  if (check == true) {
    interpreter.EnterContext<RuntimeContextType::IF>();
    er = if_block_->Execute(interpreter);
  } else {
    if (else_block_ != nullptr) {
      interpreter.EnterContext<RuntimeContextType::ELSE>();
      er = else_block_->Execute(interpreter);
    } else {
      er = ExecuteResult::Next();
    }
  }
  interpreter.LeaveContext();
  if (er.state_ == ExecuteState::Next || er.state_ == ExecuteState::Break) {
    er.state_ = ExecuteState::Next;
  }
  // 如果返回continue或者return的话，需要上层调用栈处理
  return er;
}

ExecuteResult WhileStmt::Execute(Interpreter& interpreter) {
  interpreter.EnterContext<RuntimeContextType::WHILE>();
  while(true) {
    auto v = check_->Calcualte(interpreter);
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
    auto v = return_->Calcualte(interpreter);
    return ExecuteResult::Return(std::move(v));
  }
  // 如果是return;语句则填充一个void类型返回，简化后续处理
  return ExecuteResult::Return(std::make_shared<VoidVariable>());
}

// 对于表达式语句而言，我们只是计算其临时变量，在下一个语句开始执行前就会销毁该变量
// 计算可能有副作用，因此不能优化掉这里
ExecuteResult ExpressionStmt::Execute(Interpreter& interpreter) {
  auto v = expr_->Calcualte(interpreter);
  return ExecuteResult::Next();
}

ExecuteResult VariableDefineStmt::Execute(Interpreter& interpreter) {
  auto& context = interpreter.GetCurrentContext();
  auto v = VariableFactory(type_->Clone(), var_name_, interpreter);
  std::vector<std::shared_ptr<Variable>> fields;
  for(auto& each : constructors_) {
    fields.push_back(each->Calcualte(interpreter));
  }
  v->ConstructByFields(fields);
  context.RegisterVariable(std::move(v));
  return ExecuteResult::Next();
}

}