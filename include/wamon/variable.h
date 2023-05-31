#pragma once

#include "wamon/type.h"

#include <string>

namespace wamon {

/*
 * Variable为运行时变量类型的基类，每个Variable一定包含一个type成员标识其类型
 * 以及一个变量名字，当其为空的时候标识匿名变量
 */
class Variable {
 public:
  explicit Variable(std::unique_ptr<Type>&& t, const std::string& name = "") : type_(std::move(t)), name_(name) {

  }

  virtual void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) = 0;

  virtual void DefaultConstruct() = 0;

  virtual std::unique_ptr<Variable> Clone() = 0;

  virtual ~Variable() = default;

  std::string GetTypeInfo() const {
    return type_->GetTypeInfo();
  }

  const std::string& GetName() const {
    return name_;
  }

 private:
  std::unique_ptr<Type> type_;
  std::string name_;
};

class VoidVariable : public Variable {
 public:
  VoidVariable() : Variable(TypeFactory<void>::Get()) {}

  void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) override {
    throw WamonExecption("VoidVariable should not call ConstructByFields method");
  }

  void DefaultConstruct() override {}

  std::unique_ptr<Variable> Clone() override {
    return std::make_unique<VoidVariable>();
  }
};

class StringVariable : public Variable {
 public:
  StringVariable(const std::string& v, const std::string& name) : Variable(TypeFactory<std::string>::Get(), name), value_(v) {

  }

  const std::string& GetValue() const { return value_; }

  void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) override {
    if (fields.size() != 1) {
      throw WamonExecption("StringVariable's ConstructByFields method error : fields.size() == {}", fields.size());
    }
    if (fields[0]->GetTypeInfo() != GetTypeInfo()) {
      throw WamonExecption("StringVariable's ConstructByFields method error, type dismatch : {} != {}", fields[0]->GetTypeInfo(), GetTypeInfo());
    }
    StringVariable* ptr = static_cast<StringVariable*>(fields[0].get());
    value_ = ptr->GetValue();
  }

  void DefaultConstruct() override {
    value_.clear();
  }

  std::unique_ptr<Variable> Clone() override {
    return std::make_unique<StringVariable>(GetValue(), "__clone__");
  }

 private:
  std::string value_;
};

class BoolVariable : public Variable {
 public:
  BoolVariable(bool v, const std::string& name) : Variable(TypeFactory<bool>::Get(), name), value_(v) {

  }

  bool GetValue() const { return value_; }

  void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) override {
    if (fields.size() != 1) {
      throw WamonExecption("BoolVariable's ConstructByFields method error : fields.size() == {}", fields.size());
    }
    if (fields[0]->GetTypeInfo() != GetTypeInfo()) {
      throw WamonExecption("BoolVariable's ConstructByFields method error, type dismatch : {} != {}", fields[0]->GetTypeInfo(), GetTypeInfo());
    }
    BoolVariable* ptr = static_cast<BoolVariable*>(fields[0].get());
    value_ = ptr->GetValue();
  }

  void DefaultConstruct() override {
    value_ = true;
  }

  std::unique_ptr<Variable> Clone() override {
    return std::make_unique<BoolVariable>(GetValue(), "__clone__");
  }

 private:
  bool value_;
};

// 不提供运行时检测，应该在静态分析阶段确定
BoolVariable* AsBoolVariable(std::shared_ptr<Variable>& v) {
  return static_cast<BoolVariable*>(v.get());
}

std::unique_ptr<Variable> VariableFactory(const Type* type, const std::string& name) {

}
}
