#include "wamon/type_checker.h"

#include <cassert>
#include <string>
#include <vector>

#include "fmt/format.h"
#include "wamon/builtin_functions.h"
#include "wamon/exception.h"
#include "wamon/function_def.h"
#include "wamon/inner_type_method.h"
#include "wamon/static_analyzer.h"
#include "wamon/token.h"
#include "wamon/topological_sort.h"

namespace wamon {

void CheckBlockStatement(TypeChecker& tc, BlockStmt* stmt) {
  for (auto& each : stmt->GetStmts()) {
    tc.CheckStatement(each.get());
  }
}

TypeChecker::TypeChecker(StaticAnalyzer& sa) : static_analyzer_(sa) {}

void TypeChecker::CheckTypes() {
  const auto& pu = static_analyzer_.GetPackageUnit();
  for (auto& each : pu.GetGlobalVariDefStmt()) {
    CheckType(each->GetType(),
              fmt::format("check global variable {}'s type {}", each->GetVarName(), each->GetType()->GetTypeInfo()));
  }
  for (auto& each : pu.GetFunctions()) {
    for (auto& param : each.second->param_list_) {
      CheckType(param.first, fmt::format("check function {} param {}'s type {}", each.first, param.second,
                                         param.first->GetTypeInfo()));
    }
    CheckType(each.second->return_type_,
              fmt::format("check function {} return type {}", each.first, each.second->return_type_->GetTypeInfo()));
  }
  for (auto& each : pu.GetStructs()) {
    for (auto& member : each.second->GetDataMembers()) {
      CheckType(member.second, fmt::format("check struct {} member {}'s type {}", each.first, member.first,
                                           member.second->GetTypeInfo()));
    }
    for (auto& member : each.second->GetMethods()) {
      for (auto& param : member->GetParamList()) {
        CheckType(param.first, fmt::format("check struct {} method {} param {}'s type {}", each.first,
                                           member->GetMethodName(), param.second, param.first->GetTypeInfo()));
      }
      CheckType(member->GetReturnType(), fmt::format("check struct {} method {} return type {}", each.first,
                                                     member->GetMethodName(), member->GetReturnType()->GetTypeInfo()));
    }
  }
}

void TypeChecker::CheckType(const std::unique_ptr<Type>& type, const std::string& context_info, bool can_be_void) {
  if (IsFuncType(type)) {
    auto func_type = static_cast<FuncType*>(type.get());
    for (auto& param : func_type->param_type_) {
      CheckType(param, context_info, false);
    }
    CheckType(func_type->return_type_, context_info, true);
    return;
  } else if (IsPtrType(type)) {
    auto ptr_type = static_cast<PointerType*>(type.get());
    CheckType(ptr_type->hold_type_, context_info, false);
    return;
  } else if (IsListType(type)) {
    auto list_type = static_cast<ListType*>(type.get());
    CheckType(list_type->hold_type_, context_info, false);
    return;
  }
  if (!IsBuiltInType(type)) {
    const PackageUnit& pu = static_analyzer_.GetPackageUnit();
    const auto& struct_def = pu.FindStruct(type->GetTypeInfo());
    if (struct_def == nullptr) {
      throw WamonTypeCheck(context_info, type->GetTypeInfo(), "invalid struct type");
    }
  } else {
    if (can_be_void == false && IsVoidType(type)) {
      throw WamonTypeCheck(context_info, type->GetTypeInfo(), "invalid void type");
    }
  }
}

void TypeChecker::CheckAndRegisterGlobalVariable() {
  const auto& global_var_def_stmts = static_analyzer_.GetGlobalVarDefStmt();
  for (const auto& each : global_var_def_stmts) {
    CheckStatement(each.get());
  }
}

void TypeChecker::CheckFunctions() {
  for (auto& each : static_analyzer_.GetFunctions()) {
    // 首先进行上下文相关的检测、表达式类型检测、语句合法性检测
    auto func_context = std::make_unique<Context>(each.second->name_);
    static_analyzer_.RegisterFuncParamsToContext(each.second->param_list_, func_context.get());
    static_analyzer_.Enter(std::move(func_context));
    CheckBlockStatement(*this, each.second->block_stmt_.get());
    static_analyzer_.Leave();
    CheckDeterministicReturn(each.second.get());
  }
}

void TypeChecker::CheckStructs() {
  const PackageUnit& pu = static_analyzer_.GetPackageUnit();
  const auto& structs = pu.GetStructs();
  auto built_in_types = GetBuiltInTypesWithoutVoid();
  // 保证不同类型的TypeInfo不相同，因此可以用其代表一个类型进行循环依赖分析
  bool all_succ = true;
  Graph<std::string> graph;
  for (const auto& each : built_in_types) {
    all_succ &= graph.AddNode(each);
  }
  for (const auto& each : structs) {
    all_succ &= graph.AddNode(each.first);
  }
  // 如果类型a依赖于类型b，插入一条a->b有向边
  for (const auto& each : structs) {
    auto dependent = each.second->GetDependent();
    for (auto& to : dependent) {
      graph.AddEdge(each.first, to);
    }
  }
  if (graph.TopologicalSort() == false) {
    throw WamonExecption("struct dependent check error");
  }
}

void TypeChecker::CheckMethods() {
  const PackageUnit& pu = static_analyzer_.GetPackageUnit();
  const auto& structs = pu.GetStructs();
  for (const auto& each : structs) {
    for (const auto& method : each.second->GetMethods()) {
      std::unique_ptr<Context> ctx = std::make_unique<Context>(each.first, method->GetMethodName());
      static_analyzer_.RegisterFuncParamsToContext(method->GetParamList(), ctx.get());
      static_analyzer_.Enter(std::move(ctx));
      CheckBlockStatement(*this, method->GetBlockStmt().get());
      static_analyzer_.Leave();
      CheckDeterministicReturn(method.get());
    }
  }
}

bool TypeChecker::IsDeterministicReturn(BlockStmt* basic_block) {
  for (auto it = basic_block->GetStmts().rbegin(); it != basic_block->GetStmts().rend(); ++it) {
    if (dynamic_cast<ReturnStmt*>((*it).get()) != nullptr) {
      return true;
    }
    auto if_stmt = dynamic_cast<IfStmt*>((*it).get());
    if (if_stmt != nullptr && if_stmt->else_block_ != nullptr && !if_stmt->else_block_->GetStmts().empty()) {
      return IsDeterministicReturn(if_stmt->if_block_.get()) && IsDeterministicReturn(if_stmt->else_block_.get());
    }
    auto block_stmt = dynamic_cast<BlockStmt*>((*it).get());
    if (block_stmt != nullptr) {
      bool deterministic_return = IsDeterministicReturn(block_stmt);
      if (deterministic_return == true) {
        return true;
      }
    }
  }
  return false;
}

void TypeChecker::CheckDeterministicReturn(FunctionDef* func) {
  if (IsSameType(func->GetReturnType(), GetVoidType())) {
    return;
  }
  if (!IsDeterministicReturn(func->block_stmt_.get())) {
    throw WamonDeterministicReturn(func->GetFunctionName());
  }
}

void TypeChecker::CheckDeterministicReturn(MethodDef* method) {
  if (IsSameType(method->GetReturnType(), GetVoidType())) {
    return;
  }
  if (!IsDeterministicReturn(method->GetBlockStmt().get())) {
    throw WamonDeterministicReturn(method->GetMethodName());
  }
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
  for (auto& each : plus_support_basic_type) {
    if (lt->GetTypeInfo() == each && rt->GetTypeInfo() == each) {
      return lt->Clone();
    }
  }
  return nullptr;
}

// - * /
std::unique_ptr<Type> CheckAndGetMMDResultType(Token op, std::unique_ptr<Type> lt, std::unique_ptr<Type> rt) {
  std::vector<std::string> mmd_support_basic_type = {
      "int",
      "double",
  };
  for (auto& each : mmd_support_basic_type) {
    if (lt->GetTypeInfo() == each && rt->GetTypeInfo() == each) {
      return lt->Clone();
    }
  }
  return nullptr;
}

// && ||
std::unique_ptr<Type> CheckAndGetLogicalResultType(Token op, std::unique_ptr<Type> lt, std::unique_ptr<Type> rt) {
  if (lt->GetTypeInfo() == "bool" && rt->GetTypeInfo() == "bool") {
    return std::make_unique<BasicType>("bool");
  }
  return nullptr;
}

std::unique_ptr<Type> CheckAndGetSSResultType(const TypeChecker& tc, BinaryExpr* binary_expr) {
  assert(binary_expr->op_ == Token::SUBSCRIPT);
  auto right_type = tc.GetExpressionType(binary_expr->right_.get());
  // 且不考虑编译期常量的支持
  if (right_type->GetTypeInfo() != "int") {
    throw WamonExecption("the list's index type should be int, but {}", right_type->GetTypeInfo());
  }
  auto left_type = tc.GetExpressionType(binary_expr->left_.get());
  auto list_type = dynamic_cast<ListType*>(left_type.get());
  if (list_type == nullptr) {
    throw WamonExecption("the object call operator[] should have list type, but {}", left_type->GetTypeInfo());
  }
  return list_type->GetHoldType();
}

std::unique_ptr<Type> CheckAndGetMemberAccessResultType(const TypeChecker& tc, BinaryExpr* binary_expr) {
  assert(binary_expr->op_ == Token::MEMBER_ACCESS);
  auto left_type = tc.GetExpressionType(binary_expr->left_.get());
  // 这里保证数据成员访问运算符第二个操作数一定是IdExpr类型
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
    return nullptr;
  }
  return std::make_unique<BasicType>("bool");
}

std::unique_ptr<Type> CheckAndGetAssignResultType(std::unique_ptr<Type> lt, std::unique_ptr<Type> rt) {
  if (IsSameType(lt, rt) == false) {
    return nullptr;
  }
  return lt->Clone();
}

// 首先尝试内置支持的运算符类型，如果失败则尝试自定义运算符重载
std::unique_ptr<Type> CheckAndGetBinaryOperatorResultType(Token op, std::unique_ptr<Type> lt, std::unique_ptr<Type> rt,
                                                          const PackageUnit& pu) {
  auto lt_copy = lt->Clone();
  auto rt_copy = rt->Clone();
  std::unique_ptr<Type> ret = nullptr;
  if (op == Token::PLUS) {
    ret = CheckAndGetPlusResultType(std::move(lt), std::move(rt));
  }
  if (op == Token::MINUS || op == Token::MULTIPLY || op == Token::DIVIDE) {
    ret = CheckAndGetMMDResultType(op, std::move(lt), std::move(rt));
  }
  if (op == Token::AND || op == Token::OR) {
    ret = CheckAndGetLogicalResultType(op, std::move(lt), std::move(rt));
  }
  if (op == Token::COMPARE) {
    ret = CheckAndGetCompareResultType(std::move(lt), std::move(rt));
  }
  if (op == Token::ASSIGN) {
    ret = CheckAndGetAssignResultType(std::move(lt), std::move(rt));
  }
  if (ret == nullptr) {
    std::vector<std::unique_ptr<Type>*> tmp = {&lt_copy, &rt_copy};
    std::string op_func_name = OperatorDef::CreateName(op, std::move(tmp));
    auto func = pu.FindFunction(op_func_name);
    if (func != nullptr) {
      return func->GetReturnType()->Clone();
    }
  }
  if (ret != nullptr) {
    return ret;
  } else {
    throw WamonExecption("invalid operand type for {} : {} and {}", GetTokenStr(op), lt_copy->GetTypeInfo(),
                         rt_copy->GetTypeInfo());
  }
}

/*
 * unary operator
 */

std::unique_ptr<Type> CheckAndGetUnaryMinusResultType(std::unique_ptr<Type> operand) {
  std::vector<std::string> supprot_unary_minus_basic_type{
      "int",
      "double",
  };
  for (auto& each : supprot_unary_minus_basic_type) {
    if (operand->GetTypeInfo() == each) {
      return operand->Clone();
    }
  }
  throw WamonExecption("invalid operand type for unary_minus, {}", operand->GetTypeInfo());
}

std::unique_ptr<Type> CheckAndGetUnaryMultiplyResultType(std::unique_ptr<Type> operand) {
  if (IsPtrType(operand) == false) {
    throw WamonExecption("invalid operand type for deref, {}", operand->GetTypeInfo());
  }
  return dynamic_cast<PointerType*>(operand.get())->hold_type_->Clone();
}

std::unique_ptr<Type> CheckAndGetUnaryAddrOfResultType(std::unique_ptr<Type> operand) {
  // 目前的类型系统中只要输入类型是合法的，就可以取地址
  auto ret = std::make_unique<PointerType>(std::move(operand));
  return ret;
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

std::unique_ptr<Type> CheckAndGetCallableReturnType(const TypeChecker& tc, const std::unique_ptr<Type>& ctype,
                                                    const FuncCallExpr* call_expr) {
  auto type = dynamic_cast<FuncType*>(ctype.get());
  assert(type != nullptr);
  if (type->param_type_.size() != call_expr->parameters_.size()) {
    throw WamonExecption("callable_object_call {} error, The number of parameters does not match : {} != {}",
                         call_expr->func_name_, type->param_type_.size(), call_expr->parameters_.size());
  }
  for (size_t arg_i = 0; arg_i < type->param_type_.size(); ++arg_i) {
    auto arg_i_type = tc.GetExpressionType(call_expr->parameters_[arg_i].get());
    if (!IsSameType(type->param_type_[arg_i], arg_i_type)) {
      throw WamonExecption("callable_object_call {} error, arg_{}'s type dismatch {} != {}", call_expr->func_name_,
                           arg_i, type->param_type_[arg_i]->GetTypeInfo(), arg_i_type->GetTypeInfo());
    }
  }
  // 类型检测成功
  return type->return_type_->Clone();
}

std::unique_ptr<Type> CheckAndGetOperatorOverrideReturnType(const TypeChecker& tc, const std::unique_ptr<Type>& ctype,
                                                            FuncCallExpr* call_expr) {
  auto struct_type = dynamic_cast<BasicType*>(ctype.get());
  if (struct_type == nullptr) {
    throw WamonExecption("invalid type {} for call op", ctype->GetTypeInfo());
  }
  std::vector<std::unique_ptr<Type>> param_types;
  for (auto& each : call_expr->parameters_) {
    param_types.emplace_back(tc.GetExpressionType(each.get()));
  }
  std::string op_func_name = OperatorDef::CreateName(Token::LEFT_PARENTHESIS, param_types);
  auto method_def = tc.GetStaticAnalyzer().FindTypeMethod(struct_type->GetTypeInfo(), op_func_name);
  if (method_def == nullptr) {
    throw WamonExecption("invalid type {} for call op", ctype->GetTypeInfo());
  }
  call_expr->method_name = op_func_name;
  return method_def->GetReturnType()->Clone();
}

std::unique_ptr<Type> CheckAndGetInnerMethodReturnType(const TypeChecker& tc, const std::unique_ptr<Type>& ctype,
                                                       const MethodCallExpr* call_expr) {
  assert(IsInnerType(ctype));
  std::vector<std::unique_ptr<Type>> params_type;
  for (auto& each : call_expr->parameters_) {
    params_type.push_back(tc.GetExpressionType(each.get()));
  }
  return InnerTypeMethod::Instance().CheckAndGetReturnType(ctype, call_expr->method_name_, params_type);
}

std::unique_ptr<Type> CheckAndGetMethodReturnType(const TypeChecker& tc, const MethodDef* method,
                                                  const MethodCallExpr* call_expr) {
  if (method->param_list_.size() != call_expr->parameters_.size()) {
    throw WamonExecption("method_call {} error, The number of parameters does not match : {} != {}",
                         call_expr->method_name_, method->param_list_.size(), call_expr->parameters_.size());
  }
  for (size_t arg_i = 0; arg_i < method->param_list_.size(); ++arg_i) {
    auto arg_i_type = tc.GetExpressionType(call_expr->parameters_[arg_i].get());
    if (!IsSameType(method->param_list_[arg_i].first, arg_i_type)) {
      throw WamonExecption("method_call {} error, arg_{}'s type dismatch {} != {}", call_expr->method_name_, arg_i,
                           method->param_list_[arg_i].first->GetTypeInfo(), arg_i_type->GetTypeInfo());
    }
  }
  // 类型检测成功
  return method->return_type_->Clone();
}

std::unique_ptr<Type> CheckAndGetFuncReturnType(const TypeChecker& tc, const FunctionDef* function,
                                                const FuncCallExpr* call_expr) {
  if (function->param_list_.size() != call_expr->parameters_.size()) {
    throw WamonExecption("func_call {} error, The number of parameters does not match : {} != {}",
                         call_expr->func_name_, function->param_list_.size(), call_expr->parameters_.size());
  }
  for (size_t arg_i = 0; arg_i < function->param_list_.size(); ++arg_i) {
    auto arg_i_type = tc.GetExpressionType(call_expr->parameters_[arg_i].get());
    if (!IsSameType(function->param_list_[arg_i].first, arg_i_type)) {
      throw WamonExecption("func_call {} error, arg_{}'s type dismatch {} != {}", call_expr->func_name_, arg_i,
                           function->param_list_[arg_i].first->GetTypeInfo(), arg_i_type->GetTypeInfo());
    }
  }
  // 类型检测成功
  return function->return_type_->Clone();
}

// 新的设计：
// 首先在符号表中查找call_expr->func_name_，分两种情况：
//   1. 找到了一个object
//   2. 找到了原生函数或者没找到
// 当是情况1时，检查该object的类型，如果是函数类型，说明它是一个callable object，callable
// object可以由原生函数构造，也可以由lambda表达式构造，也可以由重载了()运算符的类型的变量构造，如果是结构体类型，则由重载了()运算符的类型的变量构造
// 当是情况2时，则首先查内建函数，如果没找到则查用户定义函数
// todo: 记录一些必要信息供运行时调用
std::unique_ptr<Type> CheckParamTypeAndGetResultTypeForFunction(const TypeChecker& tc, FuncCallExpr* call_expr) {
  std::unique_ptr<Type> find_type;
  auto find_result = tc.GetStaticAnalyzer().FindNameAndType(call_expr->func_name_, find_type);
  if (find_result == FindNameResult::OBJECT) {
    if (!IsFuncType(find_type)) {
      // 尝试重载的()运算符
      call_expr->type = FuncCallExpr::FuncCallType::OPERATOR_OVERRIDE;
      return CheckAndGetOperatorOverrideReturnType(tc, find_type, call_expr);
    } else {
      call_expr->type = FuncCallExpr::FuncCallType::CALLABLE;
      return CheckAndGetCallableReturnType(tc, find_type, call_expr);
    }
  } else {
    if (BuiltinFunctions::Instance().Find(call_expr->func_name_)) {
      call_expr->type = FuncCallExpr::FuncCallType::BUILT_IN_FUNC;
      std::vector<std::unique_ptr<Type>> param_types;
      for (auto& each : call_expr->parameters_) {
        param_types.push_back(tc.GetExpressionType(each.get()));
      }
      return BuiltinFunctions::Instance().TypeCheck(call_expr->func_name_, param_types);
    } else {
      if (find_result == FindNameResult::NONE) {
        throw WamonExecption("invalid function name {} , not found it", call_expr->func_name_);
      }
      call_expr->type = FuncCallExpr::FuncCallType::FUNC;
      auto func = tc.GetStaticAnalyzer().FindFunction(call_expr->func_name_);
      return CheckAndGetFuncReturnType(tc, func, call_expr);
    }
  }
}

std::unique_ptr<Type> CheckParamTypeAndGetResultTypeForMethod(const TypeChecker& tc, MethodCallExpr* method_call_expr) {
  std::unique_ptr<Type> find_type;
  auto find_result = tc.GetStaticAnalyzer().FindNameAndType(method_call_expr->id_name_, find_type);
  if (find_result != FindNameResult::OBJECT) {
    throw WamonExecption("CheckParamTypeAndGetResultTypeForMethod error, not find ident's type");
  }
  if (IsInnerType(find_type)) {
    return CheckAndGetInnerMethodReturnType(tc, find_type, method_call_expr);
  }
  // if not find, throw exception
  auto methoddef =
      tc.GetStaticAnalyzer().GetPackageUnit().FindTypeMethod(find_type->GetTypeInfo(), method_call_expr->method_name_);
  return CheckAndGetMethodReturnType(tc, methoddef, method_call_expr);
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
    return CheckAndGetBinaryOperatorResultType(tmp->op_, std::move(left_type), std::move(right_type),
                                               GetStaticAnalyzer().GetPackageUnit());
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
  if (auto tmp = dynamic_cast<MethodCallExpr*>(expr)) {
    return CheckParamTypeAndGetResultTypeForMethod(*this, tmp);
  }
  auto tmp = dynamic_cast<IdExpr*>(expr);
  if (tmp == nullptr) {
    throw WamonExecption("type check error, invalid expression type.");
  }
  // 在上下文栈上向上查找这个id对应的类型并返回
  IdExpr::Type type = IdExpr::Type::Invalid;
  auto ret = static_analyzer_.GetTypeByName(tmp->id_name_, type);
  tmp->type_ = type;
  return ret;
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
      throw WamonExecption("for_stmt's check expr should return the value which has bool type, but {}",
                           check_type->GetTypeInfo());
    }
    CheckBlockStatement(*this, tmp->block_.get());
    return;
  }
  if (auto tmp = dynamic_cast<IfStmt*>(stmt)) {
    auto check_type = GetExpressionType(tmp->check_.get());
    if (!IsBoolType(check_type)) {
      throw WamonExecption("if_stmt's check expr should return the value which has bool type, but {}",
                           check_type->GetTypeInfo());
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
      throw WamonExecption("while_stmt's check expr should return the value which has bool type, but {}",
                           check_type->GetTypeInfo());
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
        throw WamonExecption("return type dismatch, {} != {}", define_return_type->GetTypeInfo(),
                             return_type->GetTypeInfo());
      }
    } else {
      if (!IsSameType(define_return_type, GetVoidType())) {
        throw WamonExecption("defined return void, but return expr has type {}", define_return_type->GetTypeInfo());
      }
    }
    return;
  }
  if (auto tmp = dynamic_cast<ExpressionStmt*>(stmt)) {
    auto type = GetExpressionType(tmp->expr_.get());
    tmp->SetExprType(std::move(type));
    return;
  }
  if (auto tmp = dynamic_cast<VariableDefineStmt*>(stmt)) {
    // 类型检测
    std::vector<std::unique_ptr<Type>> params_type;
    CheckType(tmp->type_,
              fmt::format("check variable {} 's type {}", tmp->GetVarName(), tmp->GetType()->GetTypeInfo()));
    for (auto& each : tmp->constructors_) {
      params_type.push_back(GetExpressionType(each.get()));
    }
    CheckCanConstructBy(static_analyzer_.GetPackageUnit(), tmp->type_, params_type);
    // 将该语句定义的变量记录到当前Context中
    static_analyzer_.GetCurrentContext()->RegisterVariable(tmp->var_name_, tmp->type_->Clone());
    return;
  }
  throw WamonExecption("invalid stmt type {}, check error", stmt->GetStmtName());
}
}  // namespace wamon