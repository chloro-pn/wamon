#pragma once

#include "wamon/type.h"

#include <string>
#include <vector>

namespace wamon {

class StructDef;

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

  std::unique_ptr<Type> GetType() const {
    return type_->Clone();
  }

  const std::string& GetName() const {
    return name_;
  }

 private:
  std::unique_ptr<Type> type_;
  std::string name_;
};

class Interpreter;
std::unique_ptr<Variable> VariableFactory(std::unique_ptr<Type> type, const std::string& name, Interpreter& interpreter);

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
    return std::make_unique<StringVariable>(GetValue(), "");
  }

 private:
  std::string value_;
};

inline StringVariable* AsStringVariable(std::shared_ptr<Variable>& v) {
  return static_cast<StringVariable*>(v.get());
}

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
    return std::make_unique<BoolVariable>(GetValue(), "");
  }

 private:
  bool value_;
};

// 不提供运行时检测，应该在静态分析阶段确定
inline BoolVariable* AsBoolVariable(std::shared_ptr<Variable>& v) {
  return static_cast<BoolVariable*>(v.get());
}

class IntVariable : public Variable {
 public:
  IntVariable(int v, const std::string& name) : Variable(TypeFactory<int>::Get(), name), value_(v) {

  }

  int GetValue() const { return value_; }

  void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) override {
    if (fields.size() != 1) {
      throw WamonExecption("IntVariable's ConstructByFields method error : fields.size() == {}", fields.size());
    }
    if (fields[0]->GetTypeInfo() != GetTypeInfo()) {
      throw WamonExecption("IntVariable's ConstructByFields method error, type dismatch : {} != {}", fields[0]->GetTypeInfo(), GetTypeInfo());
    }
    IntVariable* ptr = static_cast<IntVariable*>(fields[0].get());
    value_ = ptr->GetValue();
  }

  void DefaultConstruct() override {
    value_ = 0;
  }

  std::unique_ptr<Variable> Clone() override {
    return std::make_unique<IntVariable>(GetValue(), "");
  }

 private:
  int value_;
};

inline IntVariable* AsIntVariable(const std::shared_ptr<Variable>& v) {
  return static_cast<IntVariable*>(v.get());
}

inline IntVariable* AsIntVariable(Variable* v) {
  return static_cast<IntVariable*>(v);
}

class DoubleVariable : public Variable {
 public:
  DoubleVariable(double v, const std::string& name) : Variable(TypeFactory<double>::Get(), name), value_(v) {

  }

  double GetValue() const { return value_; }

  void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) override {
    if (fields.size() != 1) {
      throw WamonExecption("DoubleVariable's ConstructByFields method error : fields.size() == {}", fields.size());
    }
    if (fields[0]->GetTypeInfo() != GetTypeInfo()) {
      throw WamonExecption("DoubleVariable's ConstructByFields method error, type dismatch : {} != {}", fields[0]->GetTypeInfo(), GetTypeInfo());
    }
    DoubleVariable* ptr = static_cast<DoubleVariable*>(fields[0].get());
    value_ = ptr->GetValue();
  }

  void DefaultConstruct() override {
    value_ = 0.0;
  }

  std::unique_ptr<Variable> Clone() override {
    return std::make_unique<DoubleVariable>(GetValue(), "");
  }
 private:
  double value_;
};

inline DoubleVariable* AsDoubleVariable(std::shared_ptr<Variable>& v) {
  return static_cast<DoubleVariable*>(v.get());
}

class ByteVariable : public Variable {
 public:
  ByteVariable(int v, const std::string& name) : Variable(TypeFactory<char>::Get(), name), value_(v) {

  }

  char GetValue() const { return value_; }

  void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) override {
    if (fields.size() != 1) {
      throw WamonExecption("ByteVariable's ConstructByFields method error : fields.size() == {}", fields.size());
    }
    if (fields[0]->GetTypeInfo() != GetTypeInfo()) {
      throw WamonExecption("ByteVariable's ConstructByFields method error, type dismatch : {} != {}", fields[0]->GetTypeInfo(), GetTypeInfo());
    }
    ByteVariable* ptr = static_cast<ByteVariable*>(fields[0].get());
    value_ = ptr->GetValue();
  }

  void DefaultConstruct() override {
    value_ = 0;
  }

  std::unique_ptr<Variable> Clone() override {
    return std::make_unique<ByteVariable>(GetValue(), "");
  }

 private:
  char value_;
};

class StructVariable : public Variable {
 public:
  StructVariable(const StructDef* sd, Interpreter& i, const std::string& name);

  std::shared_ptr<Variable> GetDataMemberByName(const std::string& name);

  void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) override;

  void DefaultConstruct() override;

  std::unique_ptr<Variable> Clone() override;

 private:
  const StructDef* def_;
  Interpreter& interpreter_;
  struct member {
    std::string name;
    std::shared_ptr<Variable> data;
  };
  std::vector<member> data_members_;
};

inline StructVariable* AsStructVariable(std::shared_ptr<Variable>& v) {
  return static_cast<StructVariable*>(v.get());
}

class CompoundVariable : public Variable {
 public:
  CompoundVariable(std::unique_ptr<Type>&& t, const std::string& name) : Variable(std::move(t), name) {

  }
};

class PointerVariable : public CompoundVariable {
 public:
  PointerVariable(std::unique_ptr<Type>&& hold_type, const std::string& name) : CompoundVariable(std::make_unique<PointerType>(std::move(hold_type)), name) {

  }

  std::shared_ptr<Variable> GetHoldVariable() {
    return obj_.lock();
  }

  std::unique_ptr<Type> GetHoldType() {
    return obj_.lock()->GetType();
  }

  void SetHoldVariable(std::shared_ptr<Variable> v) {
    obj_ = v;
  }

  void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) override;

  void DefaultConstruct() override;

  std::unique_ptr<Variable> Clone() override;

 private:
  std::weak_ptr<Variable> obj_;
};

inline PointerVariable* AsPointerVariable(const std::shared_ptr<Variable>& v) {
  return static_cast<PointerVariable*>(v.get());
}

class ListVariable : public CompoundVariable {
 public:
  ListVariable(std::unique_ptr<Type>&& element_type, const std::string& name) : CompoundVariable(std::make_unique<ListType>(element_type->Clone()), name),
                                                                                element_type_(std::move(element_type)) {

  }

  void PushBack(std::shared_ptr<Variable> element);

  void PopBack();

  size_t Size() const {
    return elements_.size();
  }

  std::shared_ptr<Variable> at(size_t i) {
    if (i >= Size()) {
      throw WamonExecption("ListVariable.at error, index {} out of range", i);
    }
    return elements_[i];
  }

  void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) override;

  void DefaultConstruct() override;

  std::unique_ptr<Variable> Clone() override;

 private:
  std::unique_ptr<Type> element_type_;
  std::vector<std::shared_ptr<Variable>> elements_;
};

}
