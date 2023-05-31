#include "wamon/ast.h"
#include "wamon/interpreter.h"

namespace wamon {

std::shared_ptr<Variable> FuncCallExpr::Calcualte(Interpreter& interpreter) {
  std::vector<std::shared_ptr<Variable>> params;
  for(auto& each : parameters_) {
    auto v = each->Calcualte(interpreter);
    params.push_back(std::move(v));
  }
}

ExecuteResult BlockStmt::Execute(Interpreter& interpreter) {
  for(auto& each : block_) {
    ExecuteResult step = each->Execute(interpreter);
    if (step.state_ != ExecuteState::Next) {
      return step;
    }
  }
  return ExecuteResult::Next();
}

ExecuteResult ForStmt::Execute(Interpreter& interpreter) {
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
      return ExecuteResult::Next();
    }
    // Result
    return er;
  }
  return ExecuteResult::Next();
}

ExecuteResult IfStmt::Execute(Interpreter& interpreter) {
  auto v = check_->Calcualte(interpreter);
  bool check = AsBoolVariable(v)->GetValue();
  ExecuteResult er = ExecuteResult::Next();
  if (check == true) {
    er = if_block_->Execute(interpreter);
  } else {
    if (else_block_ != nullptr) {
      er = else_block_->Execute(interpreter);
    } else {
      er = ExecuteResult::Next();
    }
  }
  return er;
}

ExecuteResult WhileStmt::Execute(Interpreter& interpreter) {
  while(true) {
    auto v = check_->Calcualte(interpreter);
    bool check = AsBoolVariable(v)->GetValue();
    if (check == false) {
      return ExecuteResult::Next();
    }
    ExecuteResult er = block_->Execute(interpreter);
    if (er.state_ == ExecuteState::Continue || er.state_ == ExecuteState::Next) {
      continue;
    } else if (er.state_ == ExecuteState::Break) {
      break;
    } else {
      return er;
    }
  }
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
  auto v = VariableFactory(type_.get(), var_name_);
  std::vector<std::shared_ptr<Variable>> fields;
  for(auto& each : constructors_) {
    fields.push_back(each->Calcualte(interpreter));
  }
  v->ConstructByFields(fields);
  context.RegisterVariable(std::move(v));
  return ExecuteResult::Next();
}

}