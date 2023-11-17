#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <cassert>

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
  friend class TypeChecker;
  friend std::unique_ptr<Type> CheckAndGetUnaryMultiplyResultType(std::unique_ptr<Type> operand);

  PointerType(std::unique_ptr<Type>&& hold_type) : hold_type_(std::move(hold_type)) {
    
  }

  void SetHoldType(std::unique_ptr<Type>&& hold_type) {
    hold_type_ = std::move(hold_type);
  }

  std::unique_ptr<Type> GetHoldType() const {
    return hold_type_->Clone();
  }

  std::string GetTypeInfo() const override { 
    return "ptr(" + hold_type_->GetTypeInfo() + ")";
  }

  std::unique_ptr<Type> Clone() const override;

 private:
  std::unique_ptr<Type> hold_type_;
};

class ListType : public CompoundType {
 public:
  friend class TypeChecker;

  ListType(std::unique_ptr<Type>&& hold_type) : hold_type_(std::move(hold_type)) {
    
  }

  void SetHoldType(std::unique_ptr<Type>&& hold_type) {
    hold_type_ = std::move(hold_type);
  }

  std::string GetTypeInfo() const override;

  std::unique_ptr<Type> Clone() const override;

  std::unique_ptr<Type> GetHoldType() const {
    return hold_type_->Clone();
  }

 private:
  std::unique_ptr<Type> hold_type_;
};

class TypeChecker;
class FuncCallExpr;

class FuncType : public CompoundType {
 public:
  friend class TypeChecker;
  friend std::unique_ptr<Type> CheckAndGetCallableReturnType(const TypeChecker& tc, const std::unique_ptr<Type>& ctype, const FuncCallExpr* call_expr);

  FuncType(std::vector<std::unique_ptr<Type>>&& param_type, std::unique_ptr<Type>&& return_type) : param_type_(std::move(param_type)), return_type_(std::move(return_type)) {
    
  }

  void SetParamTypeAndReturnType(std::vector<std::unique_ptr<Type>>&& param_type, std::unique_ptr<Type>&& return_type) {
    param_type_ = std::move(param_type);
    return_type_ = std::move(return_type);
  }

  const std::vector<std::unique_ptr<Type>>& GetParamType() {
    return param_type_;
  }

  const std::unique_ptr<Type>& GetReturnType() {
    return return_type_;
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
    assert(return_type_ != nullptr);
    ret += " -> ";
    ret += return_type_->GetTypeInfo();
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

inline bool IsListType(const std::unique_ptr<Type>& type) {
  return dynamic_cast<ListType*>(type.get()) != nullptr;
}

inline bool IsBasicType(const std::unique_ptr<Type>& type) {
  return dynamic_cast<BasicType*>(type.get()) != nullptr;
}

inline bool IsStringType(const std::unique_ptr<Type>& type) {
  return type->GetTypeInfo() == "string";
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

inline std::unique_ptr<Type> GetHoldType(const std::unique_ptr<Type>& ptrtype) {
  assert(IsPtrType(ptrtype));
  return dynamic_cast<PointerType*>(ptrtype.get())->GetHoldType();
}

inline std::unique_ptr<Type> GetElementType(const std::unique_ptr<Type>& type) {
  assert(IsListType(type));
  return dynamic_cast<ListType*>(type.get())->GetHoldType();
}

inline std::vector<std::unique_ptr<Type>> GetParamType(const std::unique_ptr<Type>& type) {
  assert(IsFuncType(type));
  auto& param_type = dynamic_cast<FuncType*>(type.get())->GetParamType();
  std::vector<std::unique_ptr<Type>> ret;
  for(auto& each :param_type) {
    ret.push_back(each->Clone());
  }
  return ret;
}

inline std::unique_ptr<Type> GetReturnType(const std::unique_ptr<Type>& type) {
  assert(IsFuncType(type));
  return dynamic_cast<FuncType*>(type.get())->GetReturnType()->Clone();
}

template <typename T>
struct TypeFactory;

template <>
struct TypeFactory<void> {
  static std::unique_ptr<Type> Get() {
    return std::make_unique<BasicType>("void");
  }
};

template <>
struct TypeFactory<std::string> {
  static std::unique_ptr<Type> Get() {
    return std::make_unique<BasicType>("string");
  }
};

template <>
struct TypeFactory<int> {
  static std::unique_ptr<Type> Get() {
    return std::make_unique<BasicType>("int");
  }
};

template <>
struct TypeFactory<double> {
  static std::unique_ptr<Type> Get() {
    return std::make_unique<BasicType>("double");
  }
};

template <>
struct TypeFactory<bool> {
  static std::unique_ptr<Type> Get() {
    return std::make_unique<BasicType>("bool");
  }
};

template <>
struct TypeFactory<char> {
  static std::unique_ptr<Type> Get() {
    return std::make_unique<BasicType>("byte");
  }
};

template <typename T>
struct TypeFactory<std::vector<T>> {
  static std::unique_ptr<Type> Get() {
    auto tmp = std::make_unique<ListType>(TypeFactory<T>::Get());
    return tmp;
  }
};

template <typename T>
struct TypeFactory<T*> {
  static std::unique_ptr<Type> Get() {
    auto tmp = std::make_unique<PointerType>(TypeFactory<T>::Get());
    return tmp;
  }
};

inline std::string GetTypeListId(const std::vector<std::unique_ptr<Type>>& type_list) {
  std::string result;
  for(auto& each : type_list) {
    result += each->GetTypeInfo();
    result += "-";
  }
  return result;
}

class PackageUnit;

void CheckCanConstructBy(const PackageUnit& pu, const std::unique_ptr<Type>& var_type, const std::vector<std::unique_ptr<Type>>& param_types);

}  // namespace wamon