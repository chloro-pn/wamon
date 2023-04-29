#include "wamon/type_checker.h"
#include "wamon/token.h"
#include "wamon/static_analyzer.h"
#include "wamon/function_def.h"
#include "wamon/exception.h"

#include <cassert>
#include <vector>
#include <string>

namespace wamon {

void CheckBlockStatement(TypeChecker& tc, BlockStmt* stmt) {
  for(auto& each : stmt->GetStmts()) {
    tc.CheckStatement(each.get());
  }
}

TypeChecker::TypeChecker(StaticAnalyzer& sa) : static_analyzer_(sa) {

}

void TypeChecker::CheckAndRegisterGlobalVariable() {
  const auto& global_var_def_stmts = static_analyzer_.GetGlobalVarDefStmt();
  for(const auto& each : global_var_def_stmts) {
    CheckStatement(each.get());
  }
}

void TypeChecker::CheckFunctions() {
  for(auto& each : static_analyzer_.GetFunctions()) {
    // 首先进行上下文相关的检测、表达式类型检测、语句合法性检测
    auto func_context = std::make_unique<Context>(each.second->name_);
    RegisterFuncParamsToContext(each.second->param_list_, func_context.get());
    static_analyzer_.Enter(std::move(func_context));
    CheckBlockStatement(*this, each.second->block_stmt_.get());
    static_analyzer_.Leave();
    // todo:
    // 还需要进行确定性return检测：多个分支情况下需要确保走任何一条分支都要有类型匹配的return语句
    CheckDeterministicReturn(each.second.get());
  }
}

void TypeChecker::CheckDeterministicReturn(FunctionDef* func) {
  
}

/* 
 * binary operator
 */

// +
std::unique_ptr<Type> CheckAndGetPlusResultType(std::unique_ptr<Type> lt, std::unique_ptr<Type> rt) {
  std::vector<std::string> plus_support_basic_type = {
    "string",
    "int",
    "double",
  };
  for(auto& each : plus_support_basic_type) {
    if (lt->GetTypeInfo() == each && rt->GetTypeInfo() == each) {
      return lt->Clone();
    }
  }
  throw WamonExecption("invalid operand type for + : {} and {}", lt->GetTypeInfo(), rt->GetTypeInfo());
}

// - * /
std::unique_ptr<Type> CheckAndGetMMDResultType(Token op, std::unique_ptr<Type> lt, std::unique_ptr<Type> rt) {
  std::vector<std::string> mmd_support_basic_type = {
    "int",
    "double",
  };
  for(auto& each : mmd_support_basic_type) {
    if (lt->GetTypeInfo() == each && rt->GetTypeInfo() == each) {
      return lt->Clone();
    }
  }
  throw WamonExecption("invalid operand type for {} : {} and {}", GetTokenStr(op), lt->GetTypeInfo(), rt->GetTypeInfo());
}

// && ||
std::unique_ptr<Type> CheckAndGetLogicalResultType(Token op, std::unique_ptr<Type> lt, std::unique_ptr<Type> rt) {
  if (lt->GetTypeInfo() == "bool" && rt->GetTypeInfo() == "bool") {
    return std::make_unique<BasicType>("bool");
  }
  throw WamonExecption("invalid operand type for {} : {} and {}", GetTokenStr(op), lt->GetTypeInfo(), rt->GetTypeInfo());
}

std::unique_ptr<Type> CheckAndGetSSResultType(const TypeChecker& tc, BinaryExpr* binary_expr) {
  assert(binary_expr->op_ == Token::SUBSCRIPT);
  auto right_type = tc.GetExpressionType(binary_expr->right_.get());
  // 且不考虑编译期常量的支持
  if (right_type->GetTypeInfo() != "int") {
    throw WamonExecption("the array's index type should be int, but {}", right_type->GetTypeInfo());
  }
  auto left_type = tc.GetExpressionType(binary_expr->left_.get());
  auto array_type = dynamic_cast<ArrayType*>(left_type.get());
  if (array_type == nullptr) {
    throw WamonExecption("the object call operator[] should have array type, but {}", left_type->GetTypeInfo());
  }
  return array_type->GetHoldType();
}

std::unique_ptr<Type> CheckAndGetMemberAccessResultType(const TypeChecker& tc, BinaryExpr* binary_expr) {
  assert(binary_expr->op_ == Token::MEMBER_ACCESS);
  auto left_type = tc.GetExpressionType(binary_expr->left_.get());
  auto right_expr = dynamic_cast<IdExpr*>(binary_expr->right_.get());
  if (right_expr == nullptr) {
    throw WamonExecption("member access's right operand should be id expr");
  }
  const std::string& id = right_expr->GetId();
  return tc.GetStaticAnalyzer().GetDataMemberType(left_type->GetTypeInfo(), id);
}

// 任何类型相同的变量都可以比较
std::unique_ptr<Type> CheckAndGetCompareResultType(std::unique_ptr<Type> lt, std::unique_ptr<Type> rt) {
  if (IsSameType(lt, rt) == false) {
    throw WamonExecption("invalid type for comapre, {} and {}", lt->GetTypeInfo(), rt->GetTypeInfo());
  }
  return std::make_unique<BasicType>("bool");
}

std::unique_ptr<Type> CheckAndGetAssignResultType(std::unique_ptr<Type> lt, std::unique_ptr<Type> rt) {
  if (IsSameType(lt, rt) == false) {
    throw WamonExecption("invalid type for comapre, {} and {}", lt->GetTypeInfo(), rt->GetTypeInfo());
  }
  return lt->Clone();
}

std::unique_ptr<Type> CheckAndGetBinaryOperatorResultType(Token op, std::unique_ptr<Type> lt, std::unique_ptr<Type> rt) {
  if (op == Token::PLUS) {
    return CheckAndGetPlusResultType(std::move(lt), std::move(rt));
  }
  if (op == Token::MINUS || op == Token::MULTIPLY || op == Token::DIVIDE) {
    return CheckAndGetMMDResultType(op, std::move(lt), std::move(rt));
  }
  if (op == Token::AND || op == Token::OR) {
    return CheckAndGetLogicalResultType(op, std::move(lt), std::move(rt));
  }
  if (op == Token::COMPARE) {
    return CheckAndGetCompareResultType(std::move(lt), std::move(rt));
  }
  if (op == Token::ASSIGN) {
    return CheckAndGetAssignResultType(std::move(lt), std::move(rt));
  }
  throw WamonExecption("operator {} is not support now", GetTokenStr(op));
}

/*
 * unary operator
 */

std::unique_ptr<Type> CheckAndGetUnaryMinusResultType(std::unique_ptr<Type> operand) {
  std::vector<std::string> supprot_unary_minus_basic_type {
    "int",
    "double",
  };
  for(auto& each : supprot_unary_minus_basic_type) {
    if (operand->GetTypeInfo() == each) {
      return operand->Clone();
    }
  }
  throw WamonExecption("invalid operand type for unary_minus, {}", operand->GetTypeInfo());
}

std::unique_ptr<Type> CheckAndGetUnaryMultiplyResultType(std::unique_ptr<Type> operand) {
  if(IsPtrType(operand) == false) {
    throw WamonExecption("invalid operand type for deref, {}", operand->GetTypeInfo());
  }
  return dynamic_cast<PointerType*>(operand.get())->hold_type_->Clone();
}

std::unique_ptr<Type> CheckAndGetUnaryAddrOfResultType(std::unique_ptr<Type> operand) {
  // 数组和函数类型不允许取地址操作
  if(IsPtrType(operand) || IsBasicType(operand)) {
    auto ret = std::make_unique<PointerType>();
    ret->SetHoldType(std::move(operand));
    return ret;
  }
  throw WamonExecption("invalid operand type for address_of, {}", operand->GetTypeInfo());
}

std::unique_ptr<Type> CheckAndGetUnaryNotResultType(std::unique_ptr<Type> operand) {
  // 目前的实现只有布尔类型支持逻辑非运算符
  if (operand->GetTypeInfo() == "bool") {
    return operand;
  }
  throw WamonExecption("invalid operand type for not, {}", operand->GetTypeInfo());
}

std::unique_ptr<Type> CheckAndGetUnaryOperatorResultType(Token op, std::unique_ptr<Type> operand_type) {
  if (op == Token::MINUS) {
    return CheckAndGetUnaryMinusResultType(std::move(operand_type));
  }
  if (op == Token::MULTIPLY) {
    return CheckAndGetUnaryMultiplyResultType(std::move(operand_type));
  }
  if (op == Token::ADDRESS_OF) {
    return CheckAndGetUnaryAddrOfResultType(std::move(operand_type));
  }
  if (op == Token::NOT) {
    return CheckAndGetUnaryNotResultType(std::move(operand_type));
  }
  throw WamonExecption("operator {} is not support now", GetTokenStr(op));
}

std::unique_ptr<Type> CheckAndGetMethodReturnType(const TypeChecker& tc, const MethodDef* method, const FuncCallExpr* call_expr) {
  if (method->param_list_.size() + 1 != call_expr->parameters_.size()) {
    throw WamonExecption("method_call {} error, The number of parameters does not match : {} != {}", 
      call_expr->func_name_,
      method->param_list_.size() + 1,
      call_expr->parameters_.size());
  }
  for (size_t arg_i = 0; arg_i < method->param_list_.size(); ++arg_i) {
    // 第一个参数是调用方
    auto arg_i_type = tc.GetExpressionType(call_expr->parameters_[arg_i + 1].get());
    if (!IsSameType(method->param_list_[arg_i].first, arg_i_type)) {
      throw WamonExecption("method_call {} error, arg_{}'s type dismatch {} != {}", 
        call_expr->func_name_, 
        arg_i,
        method->param_list_[arg_i].first->GetTypeInfo(),
        arg_i_type->GetTypeInfo());
    }
  }
  // 类型检测成功
  return method->return_type_->Clone();
}

std::unique_ptr<Type> CheckAndGetFuncReturnType(const TypeChecker& tc, const FunctionDef* function, const FuncCallExpr* call_expr) {
  if (function->param_list_.size() != call_expr->parameters_.size()) {
    throw WamonExecption("func_call {} error, The number of parameters does not match : {} != {}", 
      call_expr->func_name_,
      function->param_list_.size(),
      call_expr->parameters_.size());
  }
  for (size_t arg_i = 0; arg_i < function->param_list_.size(); ++arg_i) {
    auto arg_i_type = tc.GetExpressionType(call_expr->parameters_[arg_i].get());
    if (!IsSameType(function->param_list_[arg_i].first, arg_i_type)) {
      throw WamonExecption("func_call {} error, arg_{}'s type dismatch {} != {}", 
        call_expr->func_name_, 
        arg_i,
        function->param_list_[arg_i].first->GetTypeInfo(),
        arg_i_type->GetTypeInfo());
    }
  }
  // 类型检测成功
  return function->return_type_->Clone();
}

// 举例 : call get_name(a, b, c); // a的类型为A.
// 首先查找类型A是否有方法 get_name(b, c)，如果有则进行类型检测，否则：
// 在全局函数表中查找名称为get_name的函数并进行类型检测
std::unique_ptr<Type> CheckParamTypeAndGetResultTypeForFunction(const TypeChecker& tc, FuncCallExpr* call_expr) {
  // 首先查找第一个参数的类型是否有同名的方法，如果有则调用这个方法
  // 如果没有则查找全局函数表
  if (call_expr->parameters_.empty() == false) {
    auto first_expr = call_expr->parameters_[0].get();
    auto type = tc.GetExpressionType(first_expr);
    auto method = tc.GetStaticAnalyzer().FindTypeMethod(type->GetTypeInfo(), call_expr->func_name_);
    if (method != nullptr) {
      // 参数类型检测并返回返回值类型
      return CheckAndGetMethodReturnType(tc, method, call_expr);
    }
  }
  auto func = tc.GetStaticAnalyzer().FindFunction(call_expr->func_name_);
  // 参数类型检测并返回返回值类型
  return CheckAndGetFuncReturnType(tc, func, call_expr);
}


std::unique_ptr<Type> TypeChecker::GetExpressionType(Expression* expr) const {
  assert(expr != nullptr);
  if (dynamic_cast<StringIteralExpr*>(expr)) {
    return std::make_unique<BasicType>(GetTokenStr(Token::STRING));
  }
  if (dynamic_cast<IntIteralExpr*>(expr)) {
    return std::make_unique<BasicType>(GetTokenStr(Token::INT));
  }
  if (dynamic_cast<DoubleIteralExpr*>(expr)) {
    return std::make_unique<BasicType>(GetTokenStr(Token::DOUBLE));
  }
  if (dynamic_cast<BoolIteralExpr*>(expr)) {
    return std::make_unique<BasicType>(GetTokenStr(Token::BOOL));
  }
  if (dynamic_cast<ByteIteralExpr*>(expr)) {
    return std::make_unique<BasicType>(GetTokenStr(Token::BYTE));
  }
  if (dynamic_cast<VoidIteralExpr*>(expr)) {
    return std::make_unique<BasicType>(GetTokenStr(Token::VOID));
  }
  if (auto tmp = dynamic_cast<BinaryExpr*>(expr)) {
    // 特殊的二元运算符，第二个操作数的类型由查询第一个运算符类型的定义得到
    if (tmp->op_ == Token::MEMBER_ACCESS) {
      return CheckAndGetMemberAccessResultType(*this, tmp);
    }
    if (tmp->op_ == Token::SUBSCRIPT) {
      return CheckAndGetSSResultType(*this, tmp);
    }
    auto left_type = GetExpressionType(tmp->left_.get());
    auto right_type = GetExpressionType(tmp->right_.get());
    return CheckAndGetBinaryOperatorResultType(tmp->op_, std::move(left_type), std::move(right_type));
  }
  if (auto tmp = dynamic_cast<UnaryExpr*>(expr)) {
    auto operand_type = GetExpressionType(tmp->operand_.get());
    return CheckAndGetUnaryOperatorResultType(tmp->op_, std::move(operand_type));
  }
  if (dynamic_cast<SelfExpr*>(expr)) {
    // 从上下文中获取self的类型
    const std::string& type_name = static_analyzer_.CheckMethodContextAndGetTypeName();
    return std::make_unique<BasicType>(type_name);
  }
  if (auto tmp = dynamic_cast<FuncCallExpr*>(expr)) {
    return CheckParamTypeAndGetResultTypeForFunction(*this, tmp);
  }
  auto tmp = dynamic_cast<IdExpr*>(expr);
  if (tmp == nullptr) {
    throw WamonExecption("type check error, invalid expression type.");
  }
  // 在上下文栈上向上查找这个id对应的类型并返回
  return static_analyzer_.GetTypeByName(tmp->id_name_);
}

void TypeChecker::CheckStatement(Statement* stmt) {
  assert(stmt != nullptr);
  if (auto tmp = dynamic_cast<BlockStmt*>(stmt)) {
    auto context = std::make_unique<Context>(Context::ContextType::BLOCK);
    static_analyzer_.Enter(std::move(context));
    CheckBlockStatement(*this, tmp);
    static_analyzer_.Leave();
    return;
  }
  if (auto tmp = dynamic_cast<ForStmt*>(stmt)) {
    // 调用GetExpressionType做类型检测，但是不关心其返回类型
    GetExpressionType(tmp->init_.get());
    auto check_type = GetExpressionType(tmp->check_.get());
    GetExpressionType(tmp->update_.get());
    if (!IsBoolType(check_type)) {
      throw WamonExecption("for_stmt's check expr should return the value which has bool type, but {}", check_type->GetTypeInfo());
    }
    CheckBlockStatement(*this, tmp->block_.get());
    return;
  }
  if (auto tmp = dynamic_cast<IfStmt*>(stmt)) {
    auto check_type = GetExpressionType(tmp->check_.get());
    if (!IsBoolType(check_type)) {
      throw WamonExecption("if_stmt's check expr should return the value which has bool type, but {}", check_type->GetTypeInfo());
    }
    CheckBlockStatement(*this, tmp->if_block_.get());
    if (tmp->else_block_ != nullptr) {
      CheckBlockStatement(*this, tmp->else_block_.get());
    }
    return;
  }
  if (auto tmp = dynamic_cast<WhileStmt*>(stmt)) {
    auto check_type = GetExpressionType(tmp->check_.get());
    if (!IsBoolType(check_type)) {
      throw WamonExecption("while_stmt's check expr should return the value which has bool type, but {}", check_type->GetTypeInfo());
    }
    CheckBlockStatement(*this, tmp->block_.get());
    return;
  }
  // continue和break语句只能在for语句和while语句中出现
  if (dynamic_cast<ContinueStmt*>(stmt) || dynamic_cast<BreakStmt*>(stmt)) {
    static_analyzer_.CheckForOrWhileContext();
    return;
  }
  if (auto tmp = dynamic_cast<ReturnStmt*>(stmt)) {
    auto define_return_type = static_analyzer_.CheckFuncOrMethodAndGetReturnType();
    if (tmp->return_ != nullptr) {
      auto return_type = GetExpressionType(tmp->return_.get());
      if (!IsSameType(return_type, define_return_type)) {
        throw WamonExecption("return type dismatch, {} != {}", define_return_type->GetTypeInfo(), return_type->GetTypeInfo());
      }
    } else {
      if (!IsSameType(define_return_type, GetVoidType())) {
        throw WamonExecption("defined return void, but return expr has type {}", define_return_type->GetTypeInfo());
      }
    }
    return;
  }
  if (auto tmp = dynamic_cast<ExpressionStmt*>(stmt)) {
    GetExpressionType(tmp->expr_.get());
    return;
  }
  if (auto tmp = dynamic_cast<VariableDefineStmt*>(stmt)) {
    // 类型检测
    std::vector<std::unique_ptr<Type>> params_type;
    for(auto& each : tmp->constructors_) {
      params_type.push_back(GetExpressionType(each.get()));
    }
    CheckCanConstructBy(static_analyzer_.GetPackageUnit(), tmp->type_, params_type);
    // 将该语句定义的变量记录到当前Context中
    static_analyzer_.GetCurrentContext()->RegisterVariable(tmp->var_name_, tmp->type_->Clone());
    return;
  }
  throw WamonExecption("invalid stmt type {}, check error", stmt->GetStmtName());
}
}