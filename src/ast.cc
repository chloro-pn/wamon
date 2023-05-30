#include "wamon/ast.h"
#include "wamon/interpreter.h"

namespace wamon {

ExecuteResult BlockStmt::Execute(Interpreter& interpreter) {
  for(auto& each : block_) {
    ExecuteResult step = each->Execute(interpreter);
    if (step.state_ != ExecuteState::Next) {
      return step;
    }
  }
  return ExecuteResult::Next();
}

ExecuteResult ReturnStmt::Execute(Interpreter& interpreter) {
  if (return_ != nullptr) {
    auto v = return_->Calcualte(interpreter);
    return ExecuteResult::Return(std::move(v));
  }
  // 如果是return;语句则填充一个void类型返回，简化后续分析
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