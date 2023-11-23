#pragma once

#include <memory>
#include <string>

#include "wamon/ast.h"
#include "wamon/token.h"
#include "wamon/type.h"

namespace wamon {

class Variable;

class OperatorDef {
  friend std::unique_ptr<MethodDef> OperatorOverrideToMethod(const std::string& type_name,
                                                             std::unique_ptr<OperatorDef>&& op);
  friend std::unique_ptr<FunctionDef> OperatorOverrideToFunc(std::unique_ptr<OperatorDef>&& op);

 public:
  explicit OperatorDef(Token op) : op_(op) {}

  void AddParamList(std::unique_ptr<Type>&& type, const std::string& var) {
    param_list_.push_back({std::move(type), var});
  }

  void SetReturnType(std::unique_ptr<Type>&& type) { return_type_ = std::move(type); }

  void SetBlockStmt(std::unique_ptr<BlockStmt>&& block_stmt) { block_stmt_ = std::move(block_stmt); }

  const char* GetOpStr() const {
    if (op_ == Token::LEFT_PARENTHESIS) {
      return "call";
    }
    return GetTokenStr(op_);
  }

  // 同一个运算符允许为不同的类型提供重载，因此需要使用参数类型列表构造一个id，本id需要保证不同的重载id不同
  std::string GetTypeListId() const {
    std::string result;
    result.reserve(16);
    for (auto& each : param_list_) {
      result += each.first->GetTypeInfo();
      result += "-";
    }
    return result;
  }

  static std::string CreateName(Token token, const std::vector<std::unique_ptr<Type>>& param_list) {
    std::string type_list_id;
    type_list_id.reserve(16);
    for (auto& each : param_list) {
      type_list_id += each->GetTypeInfo();
      type_list_id += "-";
    }
    return std::string("__op_") + (token == Token::LEFT_PARENTHESIS ? "call" : GetTokenStr(token)) + type_list_id;
  }

  // 避免不必要的临时对象生成
  static std::string CreateName(Token token, std::vector<std::unique_ptr<Type>*>&& param_list) {
    std::string type_list_id;
    type_list_id.reserve(16);
    for (auto& each : param_list) {
      type_list_id += (*each)->GetTypeInfo();
      type_list_id += "-";
    }
    return std::string("__op_") + (token == Token::LEFT_PARENTHESIS ? "call" : GetTokenStr(token)) + type_list_id;
  }

  static std::string CreateName(Token token, const std::vector<std::shared_ptr<Variable>>& param_list);

  static std::string CreateName(const std::unique_ptr<OperatorDef>& op) {
    return std::string("__op_") + op->GetOpStr() + op->GetTypeListId();
  }

 private:
  // 注意，因为允许()重载，而()并不作为二元运算符，因此这里使用(的token代替，这并不会引起歧义
  Token op_;
  std::vector<std::pair<std::unique_ptr<Type>, std::string>> param_list_;
  std::unique_ptr<Type> return_type_;
  std::unique_ptr<BlockStmt> block_stmt_;
};

}  // namespace wamon