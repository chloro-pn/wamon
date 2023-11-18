#pragma once

#include <memory>
#include <string>
#include <vector>

#include "wamon/token.h"
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

class TypeChecker;
class Type;
class MethodDef;
class FunctionDef;

class FuncCallExpr : public Expression {
 public:
  friend std::unique_ptr<Type> CheckParamTypeAndGetResultTypeForFunction(const TypeChecker& sa, FuncCallExpr* call_expr);
  friend std::unique_ptr<Type> CheckAndGetMethodReturnType(const TypeChecker& tc, const MethodDef* method, const FuncCallExpr* call_expr);
  friend std::unique_ptr<Type> CheckAndGetFuncReturnType(const TypeChecker& tc, const FunctionDef* function, const FuncCallExpr* call_expr);
  friend std::unique_ptr<Type> CheckAndGetCallableReturnType(const TypeChecker& tc, const std::unique_ptr<Type>& ctype, const FuncCallExpr* call_expr);
  friend std::unique_ptr<Type> CheckAndGetOperatorOverrideReturnType(const TypeChecker& tc, const std::unique_ptr<Type>& ctype, FuncCallExpr* call_expr);
  
  void SetFuncName(const std::string& func_name) { func_name_ = func_name; }

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
  std::string func_name_;
  std::vector<std::unique_ptr<Expression>> parameters_;
};

class MethodCallExpr : public Expression {
 public:
  friend std::unique_ptr<Type> CheckParamTypeAndGetResultTypeForMethod(const TypeChecker& tc, MethodCallExpr* method_call_expr);
  friend std::unique_ptr<Type> CheckAndGetMethodReturnType(const TypeChecker& tc, const MethodDef* method, const MethodCallExpr* call_expr);
  
   void SetIdName(const std::string& id_name) { id_name_ = id_name; }

   void SetMethodName(const std::string& method_name) {
    method_name_ = method_name;
   }

   void SetParameters(std::vector<std::unique_ptr<Expression>>&& param) { parameters_ = std::move(param); }

   std::shared_ptr<Variable> Calculate(Interpreter& interpreter) override;
 private:
  std::string id_name_;
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

  void SetOp(Token op) { op_ = op; }

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

  void SetOperand(std::unique_ptr<Expression>&& operand) { operand_ = std::move(operand); }
  
  std::shared_ptr<Variable> Calculate(Interpreter& interpreter) override;

 private:
  std::unique_ptr<Expression> operand_;
  Token op_;
};

class IdExpr : public Expression {
 public:
  friend class TypeChecker;
  void SetId(const std::string& id) { id_name_ = id; }

  const std::string& GetId() const {
    return id_name_;
  }

  enum class Type {
    Variable,
    Function,
    BUiltinFunc,
    Invalid,
  };

  Type type_ = Type::Invalid;

  std::shared_ptr<Variable> Calculate(Interpreter& interpreter) override;

 private:
  std::string id_name_;
};

class StringIteralExpr : public Expression {
 public:
  void SetStringIter(const std::string& str) { str_ = str; }

  std::shared_ptr<Variable> Calculate(Interpreter& interpreter) override {
    return std::make_shared<StringVariable>(str_, "");
  }

 private:
  std::string str_;
};

class IntIteralExpr : public Expression {
 public:

  void SetIntIter(const int64_t& n) { num_ = n; }

  std::shared_ptr<Variable> Calculate(Interpreter& interpreter) override {
    return std::make_shared<IntVariable>(num_, "");
  }

 private:
  int64_t num_;
};

class DoubleIteralExpr : public Expression {
 public:
  void SetDoubleIter(const double& d) { d_ = d; }

  std::shared_ptr<Variable> Calculate(Interpreter& interpreter) override {
    // not implemented now
    return std::make_shared<DoubleVariable>(d_, "");
  }

 private:
  double d_;
};

class BoolIteralExpr : public Expression {
 public:
  void SetBoolIter(bool b) { b_ = b; }

  std::shared_ptr<Variable> Calculate(Interpreter& interpreter) override {
    return std::make_shared<BoolVariable>(b_, "");
  }

 private:
  bool b_;
};

class ByteIteralExpr : public Expression {
 public:
  void SetByteIter(uint8_t byte) { byte_ = byte; }

  std::shared_ptr<Variable> Calculate(Interpreter& interpreter) override {
    // not implemented now
    return std::make_shared<ByteVariable>(byte_, "");
  }

 private:
  uint8_t byte_;
};

class VoidIteralExpr : public Expression {
 public:
 std::shared_ptr<Variable> Calculate(Interpreter& interpreter) override {
    return std::make_shared<VoidVariable>();
 }
};

class SelfExpr : public Expression {
 public:
  std::shared_ptr<Variable> Calculate(Interpreter& interpreter) override;
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
  void SetBlock(std::vector<std::unique_ptr<Statement>>&& block) {
    block_ = std::move(block);
  }

  std::string GetStmtName() override {
    return "block_stmt";
  }

  const std::vector<std::unique_ptr<Statement>>& GetStmts() const {
    return block_;
  }

  ExecuteResult Execute(Interpreter&) override;

 private:
  std::vector<std::unique_ptr<Statement>> block_;
};

class ForStmt : public Statement {
 public:
  friend class TypeChecker;

  void SetInit(std::unique_ptr<Expression>&& init) {
    init_ = std::move(init);
  }

  void SetCheck(std::unique_ptr<Expression>&& check) {
    check_ = std::move(check);
  }

  void SetUpdate(std::unique_ptr<Expression>&& update) {
    update_ = std::move(update);
  }

  void SetBlock(std::unique_ptr<BlockStmt>&& block) {
    block_ = std::move(block);
  }

  std::string GetStmtName() override {
    return "for_stmt";
  }

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

  void SetCheck(std::unique_ptr<Expression>&& check) {
    check_ = std::move(check);
  }

  void SetIfStmt(std::unique_ptr<BlockStmt>&& if_block) {
    if_block_ = std::move(if_block);
  }

  void SetElseStmt(std::unique_ptr<BlockStmt>&& else_block) {
    else_block_ = std::move(else_block);
  }

  std::string GetStmtName() override {
    return "if_stmt";
  }

  ExecuteResult Execute(Interpreter&) override;
  
 private:
  std::unique_ptr<Expression> check_;
  std::unique_ptr<BlockStmt> if_block_;
  std::unique_ptr<BlockStmt> else_block_;
};

class WhileStmt : public Statement {
 public:
  friend class TypeChecker;

  void SetCheck(std::unique_ptr<Expression>&& check) {
    check_ = std::move(check);
  }

  void SetBlock(std::unique_ptr<BlockStmt>&& block) {
    block_ = std::move(block);
  }

  std::string GetStmtName() override {
    return "while_stmt";
  }

  ExecuteResult Execute(Interpreter&) override;

 private:
  std::unique_ptr<Expression> check_;
  std::unique_ptr<BlockStmt> block_;
};

class BreakStmt : public Statement {
 public:
  std::string GetStmtName() override {
    return "break_stmt";
  }

  ExecuteResult Execute(Interpreter&) override {
    return ExecuteResult::Break();
  }
};

class ContinueStmt : public Statement {
 public:
  std::string GetStmtName() override {
    return "continue_stmt";
  }

  ExecuteResult Execute(Interpreter&) override {
    return ExecuteResult::Continue();
  }
};

class ReturnStmt : public Statement {
 public:
  friend class TypeChecker;

  void SetReturn(std::unique_ptr<Expression>&& ret) {
    return_ = std::move(ret);
  }

  std::string GetStmtName() override {
    return "return_stmt";
  }

  ExecuteResult Execute(Interpreter&) override;
  
 private:
  std::unique_ptr<Expression> return_;
};

class ExpressionStmt : public Statement {
 public:
  friend class TypeChecker;
   
  void SetExpr(std::unique_ptr<Expression>&& expr) { expr_ = std::move(expr); }
  
  // 由语义分析阶段完成
  void SetExprType(std::unique_ptr<Type>&& type) {
    expr_type_ = std::move(type);
  }

  std::string GetStmtName() override {
    return "expr_stmt";
  }

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

// let var_name_ = type_(constructors_)
class VariableDefineStmt : public Statement {
 public:
  friend class TypeChecker;

  void SetType(std::unique_ptr<Type>&& type) { type_ = std::move(type); }

  void SetVarName(const std::string& var_name) { var_name_ = var_name; }

  void SetConstructors(std::vector<std::unique_ptr<Expression>>&& cos) {
    constructors_ = std::move(cos);
  }

  const std::unique_ptr<Type>& GetType() const {
    return type_;
  }

  const std::string& GetVarName() const {
    return var_name_;
  }

  std::string GetStmtName() override {
    return "var_def_stmt";
  }

  ExecuteResult Execute(Interpreter&) override;
  
 private:
  std::unique_ptr<Type> type_;
  std::string var_name_;
  std::vector<std::unique_ptr<Expression>> constructors_;
};

}  // namespace wamon