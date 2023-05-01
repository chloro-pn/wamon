#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "wamon/token.h"

/*
 * 类型系统：
 * - 基本类型：
 *   - 内置类型
 *     - int
 *     - string
 *     - byte
 *     - double
 *     - bool
 *     - void
 *   - 结构体类型
 * - 复合类型(类型加工器):
 *   - 指针类型 *       int*
 *   - 数组类型 [num]   int[2]
 *   - 函数类型 <-      int <- (type_list)
 * 
 */
namespace wamon {

class Type {
 public:
  virtual std::string GetTypeInfo() const = 0;
  virtual bool IsBasicType() const = 0;
  virtual std::unique_ptr<Type> Clone() const = 0;
};

class BasicType : public Type {
 public:
  explicit BasicType(const std::string& type_name) : type_name_(type_name) {}

  std::string GetTypeInfo() const override { return type_name_; }

  bool IsBasicType() const override { return true; }

  std::unique_ptr<Type> Clone() const override;

 private:
  std::string type_name_;
};

class CompoundType : public Type {
 public:
  bool IsBasicType() const override { return false; }
};

class PointerType : public CompoundType {
 public:
  friend std::unique_ptr<Type> CheckAndGetUnaryMultiplyResultType(std::unique_ptr<Type> operand);

  void SetHoldType(std::unique_ptr<Type>&& hold_type) {
    hold_type_ = std::move(hold_type);
  }

  std::string GetTypeInfo() const override { 
    return "ptr(" + hold_type_->GetTypeInfo() + ")";
  }

  std::unique_ptr<Type> Clone() const override;

 private:
  std::unique_ptr<Type> hold_type_;
};

class IntIteralExpr;

class ArrayType : public CompoundType {
 public:
  void SetHoldType(std::unique_ptr<Type>&& hold_type) {
    hold_type_ = std::move(hold_type);
  }

  void SetCount(std::unique_ptr<IntIteralExpr>&& count);

  int64_t GetCount() const;

  std::string GetTypeInfo() const override;

  std::unique_ptr<Type> Clone() const override;

  std::unique_ptr<Type> GetHoldType() const {
    return hold_type_->Clone();
  }

 private:
  std::unique_ptr<IntIteralExpr> count_expr_;
  std::unique_ptr<Type> hold_type_;
};

class FuncType : public CompoundType {
 public:
  void SetParamTypeAndReturnType(std::vector<std::unique_ptr<Type>>&& param_type, std::unique_ptr<Type>&& return_type) {
    param_type_ = std::move(param_type);
    return_type_ = std::move(return_type);
  }

  std::string GetTypeInfo() const override {
    std::string ret("f((");
    for(const auto& each : param_type_) {
      ret += each->GetTypeInfo();
      ret += ", ";
    }
    if (ret.empty() == false && ret.back() != '(') {
      ret.pop_back();
      ret.pop_back();
    }
    ret.push_back(')');
    if (return_type_ != nullptr) {
      ret += " -> ";
      ret += return_type_->GetTypeInfo();
    }
    ret.push_back(')');
    return ret;
  }

  std::unique_ptr<Type> Clone() const override;

 private:
  std::unique_ptr<Type> return_type_;
  std::vector<std::unique_ptr<Type>> param_type_;
};

inline bool IsSameType(const std::unique_ptr<Type>& lt, const std::unique_ptr<Type>& rt) {
  return lt->GetTypeInfo() == rt->GetTypeInfo();
}

inline bool IsPtrType(const std::unique_ptr<Type>& type) {
  return dynamic_cast<PointerType*>(type.get()) != nullptr;
}

inline bool IsFuncType(const std::unique_ptr<Type>& type) {
  return dynamic_cast<FuncType*>(type.get()) != nullptr;
}

inline bool IsArrayType(const std::unique_ptr<Type>& type) {
  return dynamic_cast<ArrayType*>(type.get()) != nullptr;
}

inline bool IsBasicType(const std::unique_ptr<Type>& type) {
  return dynamic_cast<BasicType*>(type.get()) != nullptr;
}

inline bool IsBoolType(const std::unique_ptr<Type>& type) {
  return type->GetTypeInfo() == "bool";
}

inline bool IsVoidType(const std::unique_ptr<Type>& type) {
  return type->GetTypeInfo() == "void";
}

inline bool IsBuiltInType(const std::unique_ptr<Type>& type) {
  return type->GetTypeInfo() == "string" || 
         type->GetTypeInfo() == "int" ||
         type->GetTypeInfo() == "double" ||
         type->GetTypeInfo() == "byte" ||
         type->GetTypeInfo() == "bool" ||
         type->GetTypeInfo() == "void";
}

inline std::vector<std::string> GetBuiltInTypesWithoutVoid() {
  return std::vector<std::string> {
    "string",
    "int",
    "double",
    "byte",
    "bool",
  };
}

inline std::unique_ptr<Type> GetVoidType() {
  return std::make_unique<BasicType>("void");
}

class PackageUnit;

void CheckCanConstructBy(const PackageUnit& pu, const std::unique_ptr<Type>& var_type, const std::vector<std::unique_ptr<Type>>& param_types);

}  // namespace wamon