#pragma once

#include <cstring>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "wamon/function_def.h"
#include "wamon/token.h"
#include "wamon/type.h"
#include "wamon/variable.h"

namespace wamon {

class Interpreter;

class AstNode {
 public:
  virtual ~AstNode() = default;
};

class Variable;

class Expression : public AstNode {
 public:
  virtual std::shared_ptr<Variable> Calculate(Interpreter& interpreter) = 0;
};

class CompilationStageExpr : public Expression {};

class TypeExpr : public CompilationStageExpr {
 public:
  std::shared_ptr<Variable> Calculate(Interpreter&) override;

  void SetType(std::unique_ptr<Type>&& type) { type_ = std::move(type); }

  const std::unique_ptr<Type>& GetType() const { return type_; }

 private:
  std::unique_ptr<Type> type_;
};

class TypeChecker;
class Type;
class MethodDef;
class FunctionDef;
class MethodCallExpr;

class FuncCallExpr : public Expression {
 public:
  friend std::unique_ptr<Type> CheckParamTypeAndGetResultTypeForFunction(const TypeChecker& sa,
                                                                         FuncCallExpr* call_expr);
  friend std::unique_ptr<Type> CheckAndGetMethodReturnType(const TypeChecker& tc, const MethodDef* method,
                                                           const MethodCallExpr* call_expr);
  friend std::unique_ptr<Type> CheckAndGetFuncReturnType(const TypeChecker& tc, const FunctionDef* function,
                                                         const FuncCallExpr* call_expr);
  friend std::unique_ptr<Type> CheckAndGetCallableReturnType(const TypeChecker& tc, const std::unique_ptr<Type>& ctype,
                                                             const FuncCallExpr* call_expr);
  friend std::unique_ptr<Type> CheckAndGetOperatorOverrideReturnType(const TypeChecker& tc,
                                                                     const std::unique_ptr<Type>& ctype,
                                                                     FuncCallExpr* call_expr);

  void SetCaller(std::unique_ptr<Expression>&& caller) { caller_ = std::move(caller); }

  void SetParameters(std::vector<std::unique_ptr<Expression>>&& param) { parameters_ = std::move(param); }

  std::vector<std::unique_ptr<Expression>>& GetParameters() { return parameters_; }

  std::shared_ptr<Variable> Calculate(Interpreter& interpreter) override;

  enum class FuncCallType {
    FUNC,
    BUILT_IN_FUNC,
    CALLABLE,
    OPERATOR_OVERRIDE,
    INVALID,
  };

  FuncCallType type = FuncCallType::INVALID;
  // only used when type == OPERATOR_OVERRIDE
  std::string method_name;

 private:
  std::unique_ptr<Expression> caller_;
  std::vector<std::unique_ptr<Expression>> parameters_;
};

class MethodCallExpr : public Expression {
 public:
  friend std::unique_ptr<Type> CheckParamTypeAndGetResultTypeForMethod(const TypeChecker& tc,
                                                                       MethodCallExpr* method_call_expr);
  friend std::unique_ptr<Type> CheckAndGetMethodReturnType(const TypeChecker& tc, const MethodDef* method,
                                                           const MethodCallExpr* call_expr);
  friend std::unique_ptr<Type> CheckAndGetInnerMethodReturnType(const TypeChecker& tc,
                                                                const std::unique_ptr<Type>& ctype,
                                                                const MethodCallExpr* call_expr);

  void SetCaller(std::unique_ptr<Expression>&& caller) { caller_ = std::move(caller); };

  void SetMethodName(const std::string& method_name) { method_name_ = method_name; }

  void SetParameters(std::vector<std::unique_ptr<Expression>>&& param) { parameters_ = std::move(param); }

  std::shared_ptr<Variable> Calculate(Interpreter& interpreter) override;

 private:
  std::unique_ptr<Expression> caller_;
  std::string method_name_;
  std::vector<std::unique_ptr<Expression>> parameters_;
};

class BinaryExpr : public Expression {
 public:
  friend class TypeChecker;
  friend std::unique_ptr<Type> CheckAndGetMemberAccessResultType(const TypeChecker& tc, BinaryExpr* binary_expr);
  friend std::unique_ptr<Type> CheckAndGetSSResultType(const TypeChecker& tc, BinaryExpr* binary_expr);

  void SetLeft(std::unique_ptr<Expression>&& left) { left_ = std::move(left); }

  void SetRight(std::unique_ptr<Expression>&& right) { right_ = std::move(right); }

  std::unique_ptr<Expression>& GetLeft() { return left_; }

  std::unique_ptr<Expression>& GetRight() { return right_; }

  void SetOp(Token op) { op_ = op; }

  Token GetOp() const { return op_; }

  std::shared_ptr<Variable> Calculate(Interpreter& interpreter) override;

 private:
  std::unique_ptr<Expression> left_;
  std::unique_ptr<Expression> right_;
  Token op_;
};

class UnaryExpr : public Expression {
 public:
  friend class TypeChecker;

  void SetOp(Token op) { op_ = op; }

  Token GetOp() const { return op_; }

  void SetOperand(std::unique_ptr<Expression>&& operand) { operand_ = std::move(operand); }

  std::unique_ptr<Expression>& GetOperand() { return operand_; }

  std::shared_ptr<Variable> Calculate(Interpreter& interpreter) override;

 private:
  std::unique_ptr<Expression> operand_;
  Token op_;
};

class IdExpr : public Expression {
 public:
  void SetId(const std::string& id) { id_name_ = id; }

  void SetPackageName(const std::string& name) { package_name_ = name; }

  const std::string& GetId() const { return id_name_; }

  const std::string& GetPackageName() const { return package_name_; }

  std::string GenerateIdent() const { return package_name_ + "$" + id_name_; }

  std::shared_ptr<Variable> Calculate(Interpreter& interpreter) override;

  enum class Type {
    Variable,
    Callable,
    Function,
    BuiltinFunc,
    Invalid,
  };

  Type type_ = Type::Invalid;

  Type GetIdType() const { return type_; }

  void SetIdType(Type type) { type_ = type; }

 private:
  std::string id_name_;
  std::string package_name_;
};

class StringIteralExpr : public Expression {
 public:
  void SetStringIter(const std::string& str) { str_ = str; }

  std::shared_ptr<Variable> Calculate(Interpreter& interpreter) override {
    return std::make_shared<StringVariable>(str_, Variable::ValueCategory::RValue, "");
  }

 private:
  std::string str_;
};

class IntIteralExpr : public Expression {
 public:
  void SetIntIter(const int64_t& n) { num_ = n; }

  std::shared_ptr<Variable> Calculate(Interpreter& interpreter) override {
    return std::make_shared<IntVariable>(static_cast<int>(num_), Variable::ValueCategory::RValue, "");
  }

 private:
  int64_t num_;
};

class DoubleIteralExpr : public Expression {
 public:
  void SetDoubleIter(const double& d) { d_ = d; }

  std::shared_ptr<Variable> Calculate(Interpreter& interpreter) override {
    // not implemented now
    return std::make_shared<DoubleVariable>(d_, Variable::ValueCategory::RValue, "");
  }

 private:
  double d_;
};

class BoolIteralExpr : public Expression {
 public:
  void SetBoolIter(bool b) { b_ = b; }

  std::shared_ptr<Variable> Calculate(Interpreter& interpreter) override {
    return std::make_shared<BoolVariable>(b_, Variable::ValueCategory::RValue, "");
  }

 private:
  bool b_;
};

class ByteIteralExpr : public Expression {
 public:
  void SetByteIter(uint8_t byte) { byte_ = byte; }

  std::shared_ptr<Variable> Calculate(Interpreter& interpreter) override {
    // not implemented now
    return std::make_shared<ByteVariable>(byte_, Variable::ValueCategory::RValue, "");
  }

 private:
  uint8_t byte_;
};

class VoidIteralExpr : public Expression {
 public:
  std::shared_ptr<Variable> Calculate(Interpreter& interpreter) override { return std::make_shared<VoidVariable>(); }
};

class SelfExpr : public Expression {
 public:
  std::shared_ptr<Variable> Calculate(Interpreter& interpreter) override;
};

class LambdaExpr : public Expression {
 public:
  static bool IsLambdaName(const std::string& name) {
    const int len = strlen("__lambda_");
    return name.size() >= len && name.substr(0, len) == "__lambda_";
  }

  std::shared_ptr<Variable> Calculate(Interpreter& interpreter) override;

  const std::string& GetLambdaName() const { return lambda_func_name_; }

  void SetLambdaFuncName(const std::string& lfn) { lambda_func_name_ = lfn; }

 private:
  // parser在解析到一个lambda时，会生成一个唯一的名字并将lambda注册为一个全局函数
  std::string lambda_func_name_;
};

class NewExpr : public Expression {
 public:
  std::shared_ptr<Variable> Calculate(Interpreter& interpreter) override;

  void SetNewType(std::unique_ptr<Type> type) { type_ = std::move(type); }

  const std::unique_ptr<Type>& GetNewType() const { return type_; }

  void SetParameters(std::vector<std::unique_ptr<Expression>>&& param) { parameters_ = std::move(param); }

  std::vector<std::unique_ptr<Expression>>& GetParameters() { return parameters_; }

 private:
  std::unique_ptr<Type> type_;
  std::vector<std::unique_ptr<Expression>> parameters_;
};

class AllocExpr : public Expression {
 public:
  std::shared_ptr<Variable> Calculate(Interpreter& interpreter) override;

  void SetAllocType(std::unique_ptr<Type> type) { type_ = std::move(type); }

  const std::unique_ptr<Type>& GetAllocType() const { return type_; }

  void SetParameters(std::vector<std::unique_ptr<Expression>>&& param) { parameters_ = std::move(param); }

  std::vector<std::unique_ptr<Expression>>& GetParameters() { return parameters_; }

 private:
  std::unique_ptr<Type> type_;
  std::vector<std::unique_ptr<Expression>> parameters_;
};

class DeallocExpr : public Expression {
 public:
  std::shared_ptr<Variable> Calculate(Interpreter& interpreter) override;

  void SetDeallocParam(std::unique_ptr<Expression>&& param) { param_ = std::move(param); }

  std::unique_ptr<Expression>& GetDeallocParam() { return param_; }

 private:
  std::unique_ptr<Expression> param_;
};

enum class ExecuteState {
  Next,
  Continue,
  Break,
  Return,
};

struct ExecuteResult {
  ExecuteState state_;
  std::shared_ptr<Variable> result_;

  static ExecuteResult Next() {
    ExecuteResult obj;
    obj.state_ = ExecuteState::Next;
    return obj;
  }

  static ExecuteResult Continue() {
    ExecuteResult obj;
    obj.state_ = ExecuteState::Continue;
    return obj;
  }

  static ExecuteResult Break() {
    ExecuteResult obj;
    obj.state_ = ExecuteState::Break;
    return obj;
  }

  static ExecuteResult Return(std::shared_ptr<Variable>&& return_value) {
    ExecuteResult obj;
    obj.state_ = ExecuteState::Return;
    obj.result_ = std::move(return_value);
    return obj;
  }
};

class Statement : public AstNode {
 public:
  virtual std::string GetStmtName() = 0;
  virtual ExecuteResult Execute(Interpreter&) = 0;
};

class BlockStmt : public Statement {
 public:
  void SetBlock(std::vector<std::unique_ptr<Statement>>&& block) { block_ = std::move(block); }

  std::string GetStmtName() override { return "block_stmt"; }

  const std::vector<std::unique_ptr<Statement>>& GetStmts() const { return block_; }

  ExecuteResult Execute(Interpreter&) override;

 private:
  std::vector<std::unique_ptr<Statement>> block_;
};

class ForStmt : public Statement {
 public:
  friend class TypeChecker;

  void SetInit(std::unique_ptr<Expression>&& init) { init_ = std::move(init); }

  void SetCheck(std::unique_ptr<Expression>&& check) { check_ = std::move(check); }

  void SetUpdate(std::unique_ptr<Expression>&& update) { update_ = std::move(update); }

  void SetBlock(std::unique_ptr<BlockStmt>&& block) { block_ = std::move(block); }

  std::string GetStmtName() override { return "for_stmt"; }

  ExecuteResult Execute(Interpreter&) override;

 private:
  std::unique_ptr<Expression> init_;
  std::unique_ptr<Expression> check_;
  std::unique_ptr<Expression> update_;
  std::unique_ptr<BlockStmt> block_;
};

class IfStmt : public Statement {
 public:
  friend class TypeChecker;

  void SetCheck(std::unique_ptr<Expression>&& check) { check_ = std::move(check); }

  void SetIfStmt(std::unique_ptr<BlockStmt>&& if_block) { if_block_ = std::move(if_block); }

  void AddElifItem(std::unique_ptr<Expression>&& check, std::unique_ptr<BlockStmt>&& block) {
    elif_item_.emplace_back(std::move(check), std::move(block));
  }

  void SetElseStmt(std::unique_ptr<BlockStmt>&& else_block) { else_block_ = std::move(else_block); }

  std::string GetStmtName() override { return "if_stmt"; }

  ExecuteResult Execute(Interpreter&) override;

 private:
  struct elif_item {
    std::unique_ptr<Expression> elif_check;
    std::unique_ptr<BlockStmt> elif_block;

    elif_item(std::unique_ptr<Expression>&& check, std::unique_ptr<BlockStmt>&& block)
        : elif_check(std::move(check)), elif_block(std::move(block)) {}
  };

  std::unique_ptr<Expression> check_;
  std::unique_ptr<BlockStmt> if_block_;
  std::vector<elif_item> elif_item_;
  std::unique_ptr<BlockStmt> else_block_;
};

class WhileStmt : public Statement {
 public:
  friend class TypeChecker;

  void SetCheck(std::unique_ptr<Expression>&& check) { check_ = std::move(check); }

  void SetBlock(std::unique_ptr<BlockStmt>&& block) { block_ = std::move(block); }

  std::string GetStmtName() override { return "while_stmt"; }

  ExecuteResult Execute(Interpreter&) override;

 private:
  std::unique_ptr<Expression> check_;
  std::unique_ptr<BlockStmt> block_;
};

class BreakStmt : public Statement {
 public:
  std::string GetStmtName() override { return "break_stmt"; }

  ExecuteResult Execute(Interpreter&) override { return ExecuteResult::Break(); }
};

class ContinueStmt : public Statement {
 public:
  std::string GetStmtName() override { return "continue_stmt"; }

  ExecuteResult Execute(Interpreter&) override { return ExecuteResult::Continue(); }
};

class ReturnStmt : public Statement {
 public:
  friend class TypeChecker;

  void SetReturn(std::unique_ptr<Expression>&& ret) { return_ = std::move(ret); }

  std::string GetStmtName() override { return "return_stmt"; }

  ExecuteResult Execute(Interpreter&) override;

 private:
  std::unique_ptr<Expression> return_;
};

class ExpressionStmt : public Statement {
 public:
  friend class TypeChecker;

  void SetExpr(std::unique_ptr<Expression>&& expr) { expr_ = std::move(expr); }

  // 由语义分析阶段完成
  void SetExprType(std::unique_ptr<Type>&& type) { expr_type_ = std::move(type); }

  std::string GetStmtName() override { return "expr_stmt"; }

  const std::unique_ptr<Type>& GetType() const {
    assert(expr_type_ != nullptr);
    return expr_type_;
  }

  ExecuteResult Execute(Interpreter&) override;

 private:
  std::unique_ptr<Expression> expr_;
  std::unique_ptr<Type> expr_type_;
};

class Type;

class VariableDefineStmt : public Statement {
 public:
  void SetType(std::unique_ptr<Type>&& type) { type_ = std::move(type); }

  void SetRefTag() { is_ref_ = true; }

  bool IsRef() const { return is_ref_; }

  void SetVarName(const std::string& var_name) { var_name_ = var_name; }

  void SetConstructors(std::vector<std::unique_ptr<Expression>>&& cos) { constructors_ = std::move(cos); }

  void SetConstructors(std::unique_ptr<Expression>&& cos) {
    constructors_.clear();
    constructors_.push_back(std::move(cos));
  }

  std::vector<std::unique_ptr<Expression>>& GetConstructors() { return constructors_; }

  const std::unique_ptr<Type>& GetType() const { return type_; }

  const std::string& GetVarName() const { return var_name_; }

  std::string GetStmtName() override { return "var_def_stmt"; }

  ExecuteResult Execute(Interpreter&) override;

 private:
  std::unique_ptr<Type> type_;
  bool is_ref_{false};
  std::string var_name_;
  std::vector<std::unique_ptr<Expression>> constructors_;
};

}  // namespace wamon
