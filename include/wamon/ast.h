#pragma once

#include <memory>
#include <string>
#include <vector>

#include "wamon/token.h"
#include "wamon/type.h"

namespace wamon {

class AstNode {
 public:
  virtual ~AstNode() = default;
};

class Expression : public AstNode {
 public:
};

class FuncCallExpr : public Expression {
 public:
 private:
  std::string func_name_;
  std::vector<std::unique_ptr<Expression>> parameters_;
};

class DataMemberExpr : public Expression {
 public:
 private:
  std::string var_name_;
  std::string data_member_name_;
};

class BinaryExpr : public Expression {
 public:
 private:
  std::unique_ptr<Expression> left_;
  std::unique_ptr<Expression> right_;
  Token op_;
};

class UnaryExpr : public Expression {
 public:
 private:
  std::unique_ptr<Expression> operand_;
  Token op_;
};

class IdExpr : public Expression {
 public:
 private:
  std::string id_name_;
};

class IndexExpr : public Expression {
 public:
 private:
  std::string var_name_;
  size_t index_;
};

class StringIteralExpr : public Expression {
 public:
 private:
  std::string str_;
};

class IntIteralExpr : public Expression {
 public:
 private:
  int64_t num_;
};

class DoubleIteralExpr : public Expression {
 public:
 private:
  double d_;
};

class BoolIteralExpr : public Expression {
 public:
 private:
  bool b_;
};

class ByteIteralExpr : public Expression {
 public:
 private:
  uint8_t byte_;
};

class Statement : public AstNode {
 public:
};

class BlockStmt : public Statement {
 public:
 private:
  std::vector<std::unique_ptr<Statement>> block_;
};

class ForStmt : public Statement {
 public:
 private:
  std::unique_ptr<Expression> init_;
  std::unique_ptr<Expression> check_;
  std::unique_ptr<Expression> update_;
  std::unique_ptr<BlockStmt> block_;
};

class IfStmt : public Statement {
 public:
 private:
  std::unique_ptr<Expression> check_;
  std::unique_ptr<BlockStmt> if_block_;
  std::unique_ptr<BlockStmt> else_block_;
};

class WhileStmt : public Statement {
 public:
 private:
  std::unique_ptr<Expression> check_;
  std::unique_ptr<BlockStmt> block_;
};

class BreakStmt : public Statement {};

class ContinueStmt : public Statement {};

class ReturnStmt : public Statement {
 public:
 private:
  std::unique_ptr<Expression> return_;
};

class ExpressionStmt : public Statement {
 public:
 private:
  std::unique_ptr<Expression> expr_;
};

// let var_name_ = type_(constructors_)
class VariableDefineStmt : public Statement {
 public:
 private:
  std::unique_ptr<Type> type_;
  std::string var_name_;
  std::vector<std::unique_ptr<Expression>> constructors_;
};

}  // namespace wamon