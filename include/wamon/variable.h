#pragma once

#include <string>
#include <vector>

#include "wamon/output.h"
#include "wamon/type.h"

namespace wamon {

class StructDef;

/*
 * Variable为运行时变量类型的基类，每个Variable一定包含一个type成员标识其类型
 * 以及一个变量名字，当其为空的时候标识匿名变量
 * 每个Variable都具有值型别(like c++)，左值或者右值
 */
class Variable {
 public:
  enum class ValueCategory {
    LValue,
    RValue,
  };

  explicit Variable(std::unique_ptr<Type>&& t, ValueCategory vc, const std::string& name = "")
      : type_(std::move(t)), name_(name), vc_(vc) {}

  virtual void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) = 0;

  virtual void DefaultConstruct() = 0;

  virtual std::unique_ptr<Variable> Clone() = 0;

  virtual bool Compare(const std::shared_ptr<Variable>& other) = 0;

  virtual void Assign(const std::shared_ptr<Variable>& other) = 0;

  virtual void Print(Output& output) = 0;

  virtual ~Variable() = default;

  std::string GetTypeInfo() const { return type_->GetTypeInfo(); }

  std::unique_ptr<Type> GetType() const { return type_->Clone(); }

  void SetName(const std::string& name) { name_ = name; }

  const std::string& GetName() const { return name_; }

  ValueCategory GetValueCategory() const { return vc_; }

  virtual void ChangeTo(ValueCategory vc) { vc_ = vc; }

  bool IsRValue() const noexcept { return vc_ == ValueCategory::RValue; }

 protected:
  void check_compare_type_match(const std::shared_ptr<Variable>& other) {
    if (GetTypeInfo() == other->GetTypeInfo()) {
      return;
    }
    throw WamonCompareTypeDismatch(GetTypeInfo(), other->GetTypeInfo());
  }

 private:
  std::unique_ptr<Type> type_;
  std::string name_;

 protected:
  ValueCategory vc_;
};

class PackageUnit;
std::unique_ptr<Variable> VariableFactory(const std::unique_ptr<Type>& type, Variable::ValueCategory vc,
                                          const std::string& name, const PackageUnit& pu);

std::unique_ptr<Variable> GetVoidVariable();

class VoidVariable : public Variable {
 public:
  explicit VoidVariable(std::unique_ptr<Type> type = TypeFactory<void>::Get())
      : Variable(std::move(type), ValueCategory::RValue) {}

  void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) override {
    throw WamonExecption("VoidVariable should not call ConstructByFields method");
  }

  void DefaultConstruct() override { throw WamonExecption("VoidVariable should not call DefaultConstruct method"); }

  bool Compare(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    return true;
  }

  void Assign(const std::shared_ptr<Variable>& other) override {
    throw WamonExecption("VoidVariable.Assign should not be called");
  }

  void Print(Output& output) override { throw WamonExecption("VoidVariable.Print should not be called"); }

  std::unique_ptr<Variable> Clone() override { return std::make_unique<VoidVariable>(); }
};

class TypeVariable : public VoidVariable {
 public:
  TypeVariable(std::unique_ptr<Type>&& type) : VoidVariable(std::move(type)) {}

  bool Compare(const std::shared_ptr<Variable>& other) override { return GetTypeInfo() == other->GetTypeInfo(); }

  std::unique_ptr<Variable> Clone() override { return std::make_unique<TypeVariable>(GetType()->Clone()); }
};

inline TypeVariable* AsTypeVariable(const std::shared_ptr<Variable>& v) { return static_cast<TypeVariable*>(v.get()); }

class StringVariable : public Variable {
 public:
  StringVariable(const std::string& v, ValueCategory vc, const std::string& name)
      : Variable(TypeFactory<std::string>::Get(), vc, name), value_(v) {}

  const std::string& GetValue() const { return value_; }

  void SetValue(const std::string& v) { value_ = v; }

  void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) override {
    if (fields.size() != 1) {
      throw WamonExecption("StringVariable's ConstructByFields method error : fields.size() == {}", fields.size());
    }
    if (fields[0]->GetTypeInfo() != GetTypeInfo()) {
      throw WamonExecption("StringVariable's ConstructByFields method error, type dismatch : {} != {}",
                           fields[0]->GetTypeInfo(), GetTypeInfo());
    }
    StringVariable* ptr = static_cast<StringVariable*>(fields[0].get());
    if (ptr->IsRValue()) {
      value_ = std::move(ptr->value_);
    } else {
      value_ = ptr->GetValue();
    }
  }

  void DefaultConstruct() override { value_.clear(); }

  std::unique_ptr<Variable> Clone() override {
    std::unique_ptr<StringVariable> ret;
    if (IsRValue()) {
      ret = std::make_unique<StringVariable>(std::move(value_), ValueCategory::RValue, "");
    } else {
      ret = std::make_unique<StringVariable>(GetValue(), ValueCategory::RValue, "");
    }
    return ret;
  }

  bool Compare(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    return value_ == static_cast<StringVariable*>(other.get())->value_;
  }

  void Assign(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    if (other->IsRValue()) {
      value_ = std::move(static_cast<StringVariable*>(other.get())->value_);
    } else {
      value_ = static_cast<StringVariable*>(other.get())->value_;
    }
  }

  void Print(Output& output) override { output.OutPutString(value_); }

 private:
  std::string value_;
};

inline StringVariable* AsStringVariable(const std::shared_ptr<Variable>& v) {
  return static_cast<StringVariable*>(v.get());
}

inline StringVariable* AsStringVariable(const std::unique_ptr<Variable>& v) {
  return static_cast<StringVariable*>(v.get());
}

class BoolVariable : public Variable {
 public:
  BoolVariable(bool v, ValueCategory vc, const std::string& name)
      : Variable(TypeFactory<bool>::Get(), vc, name), value_(v) {}

  bool GetValue() const { return value_; }

  void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) override {
    if (fields.size() != 1) {
      throw WamonExecption("BoolVariable's ConstructByFields method error : fields.size() == {}", fields.size());
    }
    if (fields[0]->GetTypeInfo() != GetTypeInfo()) {
      throw WamonExecption("BoolVariable's ConstructByFields method error, type dismatch : {} != {}",
                           fields[0]->GetTypeInfo(), GetTypeInfo());
    }
    BoolVariable* ptr = static_cast<BoolVariable*>(fields[0].get());
    value_ = ptr->GetValue();
  }

  void DefaultConstruct() override { value_ = true; }

  std::unique_ptr<Variable> Clone() override {
    auto ret = std::make_unique<BoolVariable>(GetValue(), ValueCategory::RValue, "");
    return ret;
  }

  bool Compare(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    return value_ == static_cast<BoolVariable*>(other.get())->value_;
  }

  void Assign(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    value_ = static_cast<BoolVariable*>(other.get())->value_;
  }

  void Print(Output& output) override { output.OutPutBool(value_); }

 private:
  bool value_;
};

// 不提供运行时检测，应该在静态分析阶段确定
inline BoolVariable* AsBoolVariable(const std::shared_ptr<Variable>& v) { return static_cast<BoolVariable*>(v.get()); }

class IntVariable : public Variable {
 public:
  IntVariable(int v, ValueCategory vc, const std::string& name)
      : Variable(TypeFactory<int>::Get(), vc, name), value_(v) {}

  int GetValue() const { return value_; }

  void SetValue(int v) { value_ = v; }

  void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) override {
    if (fields.size() != 1) {
      throw WamonExecption("IntVariable's ConstructByFields method error : fields.size() == {}", fields.size());
    }
    if (fields[0]->GetTypeInfo() != GetTypeInfo()) {
      throw WamonExecption("IntVariable's ConstructByFields method error, type dismatch : {} != {}",
                           fields[0]->GetTypeInfo(), GetTypeInfo());
    }
    IntVariable* ptr = static_cast<IntVariable*>(fields[0].get());
    value_ = ptr->GetValue();
  }

  void DefaultConstruct() override { value_ = 0; }

  std::unique_ptr<Variable> Clone() override {
    auto ret = std::make_unique<IntVariable>(GetValue(), ValueCategory::RValue, "");
    return ret;
  }

  bool Compare(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    return value_ == static_cast<IntVariable*>(other.get())->value_;
  }

  void Assign(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    value_ = static_cast<IntVariable*>(other.get())->value_;
  }

  void Print(Output& output) override { output.OutPutInt(value_); }

 private:
  int value_;
};

inline IntVariable* AsIntVariable(const std::shared_ptr<Variable>& v) { return static_cast<IntVariable*>(v.get()); }

inline IntVariable* AsIntVariable(const std::unique_ptr<Variable>& v) { return static_cast<IntVariable*>(v.get()); }

inline IntVariable* AsIntVariable(Variable* v) { return static_cast<IntVariable*>(v); }

class DoubleVariable : public Variable {
 public:
  DoubleVariable(double v, ValueCategory vc, const std::string& name)
      : Variable(TypeFactory<double>::Get(), vc, name), value_(v) {}

  double GetValue() const { return value_; }

  void SetValue(double d) { value_ = d; }

  void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) override {
    if (fields.size() != 1) {
      throw WamonExecption("DoubleVariable's ConstructByFields method error : fields.size() == {}", fields.size());
    }
    if (fields[0]->GetTypeInfo() != GetTypeInfo()) {
      throw WamonExecption("DoubleVariable's ConstructByFields method error, type dismatch : {} != {}",
                           fields[0]->GetTypeInfo(), GetTypeInfo());
    }
    DoubleVariable* ptr = static_cast<DoubleVariable*>(fields[0].get());
    value_ = ptr->GetValue();
  }

  void DefaultConstruct() override { value_ = 0.0; }

  std::unique_ptr<Variable> Clone() override {
    auto ret = std::make_unique<DoubleVariable>(GetValue(), ValueCategory::RValue, "");
    return ret;
  }

  bool Compare(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    return value_ == static_cast<DoubleVariable*>(other.get())->value_;
  }

  void Assign(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    value_ = static_cast<DoubleVariable*>(other.get())->value_;
  }

  void Print(Output& output) override { output.OutPutDouble(value_); }

 private:
  double value_;
};

inline DoubleVariable* AsDoubleVariable(const std::shared_ptr<Variable>& v) {
  return static_cast<DoubleVariable*>(v.get());
}

inline DoubleVariable* AsDoubleVariable(const std::unique_ptr<Variable>& v) {
  return static_cast<DoubleVariable*>(v.get());
}

class ByteVariable : public Variable {
 public:
  ByteVariable(unsigned char v, ValueCategory vc, const std::string& name)
      : Variable(TypeFactory<unsigned char>::Get(), vc, name), value_(v) {}

  unsigned char GetValue() const { return value_; }

  void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) override {
    if (fields.size() != 1) {
      throw WamonExecption("ByteVariable's ConstructByFields method error : fields.size() == {}", fields.size());
    }
    if (fields[0]->GetTypeInfo() != GetTypeInfo()) {
      throw WamonExecption("ByteVariable's ConstructByFields method error, type dismatch : {} != {}",
                           fields[0]->GetTypeInfo(), GetTypeInfo());
    }
    ByteVariable* ptr = static_cast<ByteVariable*>(fields[0].get());
    value_ = ptr->GetValue();
  }

  void DefaultConstruct() override { value_ = 0; }

  std::unique_ptr<Variable> Clone() override {
    auto ret = std::make_unique<ByteVariable>(GetValue(), ValueCategory::RValue, "");
    return ret;
  }

  bool Compare(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    return value_ == static_cast<ByteVariable*>(other.get())->value_;
  }

  void Assign(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    value_ = static_cast<ByteVariable*>(other.get())->value_;
  }

  void Print(Output& output) override { output.OutPutByte(value_); }

 private:
  unsigned char value_;
};

inline ByteVariable* AsByteVariable(const std::shared_ptr<Variable>& v) { return static_cast<ByteVariable*>(v.get()); }

class StructVariable : public Variable {
 public:
  StructVariable(const StructDef* sd, ValueCategory vc, const PackageUnit& i, const std::string& name);

  std::shared_ptr<Variable> GetDataMemberByName(const std::string& name);

  std::shared_ptr<Variable> GetTraitObj() const { return trait_proxy_; }

  void UpdateDataMemberByName(const std::string& name, std::shared_ptr<Variable> data);

  void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) override;

  void DefaultConstruct() override;

  std::unique_ptr<Variable> Clone() override;

 private:
  static bool trait_compare(StructVariable* lv, StructVariable* rv);

  static void trait_assign(StructVariable* lv, StructVariable* rv);

  void check_trait_not_null(const char* file, int line) {
    if (trait_proxy_ == nullptr) {
      throw WamonExecption("check trait not null feiled, {} {}", file, line);
    }
  }

  // 目前仅支持相同类型的trait间的比较和赋值
  bool Compare(const std::shared_ptr<Variable>& other) override;

  void Assign(const std::shared_ptr<Variable>& other) override;

  void ChangeTo(ValueCategory vc) override;

  void Print(Output& output) override;

 private:
  const StructDef* def_;
  const PackageUnit& pu_;
  struct member {
    std::string name;
    std::shared_ptr<Variable> data;
  };
  std::vector<member> data_members_;
  // only valid when def_ is struct trait.
  std::shared_ptr<Variable> trait_proxy_;
};

inline StructVariable* AsStructVariable(const std::shared_ptr<Variable>& v) {
  return static_cast<StructVariable*>(v.get());
}

class CompoundVariable : public Variable {
 public:
  CompoundVariable(std::unique_ptr<Type>&& t, ValueCategory vc, const std::string& name)
      : Variable(std::move(t), vc, name) {}
};

class PointerVariable : public CompoundVariable {
 public:
  PointerVariable(std::unique_ptr<Type>&& hold_type, ValueCategory vc, const std::string& name)
      : CompoundVariable(std::make_unique<PointerType>(std::move(hold_type)), vc, name) {}

  std::shared_ptr<Variable> GetHoldVariable() { return obj_.lock(); }

  std::unique_ptr<Type> GetHoldType() { return obj_.lock()->GetType(); }

  void SetHoldVariable(std::shared_ptr<Variable> v) { obj_ = v; }

  void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) override;

  void DefaultConstruct() override;

  std::unique_ptr<Variable> Clone() override;

  bool Compare(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    return obj_.lock() == static_cast<PointerVariable*>(other.get())->obj_.lock();
  }

  void Assign(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    obj_ = static_cast<PointerVariable*>(other.get())->obj_;
  }

  void Print(Output& output) override {
    auto v = obj_.lock();
    if (v) {
      output.OutPutString("pointer to : ");
      v->Print(output);
    } else {
      output.OutPutString("nullptr");
    }
  }

 private:
  std::weak_ptr<Variable> obj_;
};

inline PointerVariable* AsPointerVariable(const std::shared_ptr<Variable>& v) {
  return static_cast<PointerVariable*>(v.get());
}

class ListVariable : public CompoundVariable {
 public:
  ListVariable(std::unique_ptr<Type>&& element_type, ValueCategory vc, const std::string& name)
      : CompoundVariable(std::make_unique<ListType>(element_type->Clone()), vc, name),
        element_type_(std::move(element_type)) {}

  void PushBack(std::shared_ptr<Variable> element);

  void PopBack();

  size_t Size() const { return elements_.size(); }

  void Resize(size_t new_size, const PackageUnit& pu) {
    size_t old_size = Size();
    if (new_size == old_size) {
      return;
    } else if (new_size < old_size) {
      elements_.resize(new_size);
    } else {
      for (size_t i = 0; i < (new_size - old_size); ++i) {
        auto v = VariableFactory(element_type_, Variable::ValueCategory::RValue, "", pu);
        v->DefaultConstruct();
        elements_.push_back(std::move(v));
      }
    }
  }

  void Insert(size_t index, std::shared_ptr<Variable> v) {
    if (index > Size()) {
      throw WamonExecption("List.Insert error, index = {}, size = {}", index, Size());
    }
    auto it = elements_.begin();
    std::advance(it, index);
    elements_.insert(it, std::move(v));
  }

  std::shared_ptr<Variable> at(size_t i) {
    if (i >= Size()) {
      throw WamonExecption("ListVariable.at error, index {} out of range", i);
    }
    return elements_[i];
  }

  std::string get_string_only_for_byte_list() {
    if (!IsByteType(element_type_)) {
      throw WamonExecption("ListVariable.get_string_only_for_byte_list can only be called by List(byte) type");
    }
    std::string ret;
    for (auto& each : elements_) {
      ret.push_back(char(AsByteVariable(each)->GetValue()));
    }
    return ret;
  }

  void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) override;

  void DefaultConstruct() override;

  std::unique_ptr<Variable> Clone() override;

  bool Compare(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    ListVariable* list_v = static_cast<ListVariable*>(other.get());
    if (elements_.size() != list_v->elements_.size()) {
      return false;
    }
    for (size_t i = 0; i < elements_.size(); ++i) {
      if (elements_[i]->Compare((list_v->elements_)[i]) == false) {
        return false;
      }
    }
    return true;
  }

  void Assign(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    ListVariable* list_v = static_cast<ListVariable*>(other.get());
    elements_.clear();
    for (size_t i = 0; i < list_v->elements_.size(); ++i) {
      PushBack(list_v->elements_[i]->Clone());
    }
  }

  void ChangeTo(ValueCategory vc) override {
    vc_ = vc;
    for (auto& each : elements_) {
      each->ChangeTo(vc);
    }
  }

  void Print(Output& output) override {
    output.OutputFormat("list ({}) size : {}", element_type_->GetTypeInfo(), elements_.size());
  }

 private:
  std::unique_ptr<Type> element_type_;
  std::vector<std::shared_ptr<Variable>> elements_;
};

inline ListVariable* AsListVariable(const std::shared_ptr<Variable>& v) { return static_cast<ListVariable*>(v.get()); }

class FunctionVariable : public CompoundVariable {
 public:
  FunctionVariable(std::vector<std::unique_ptr<Type>>&& param, std::unique_ptr<Type>&& ret, ValueCategory vc,
                   const std::string& name)
      : CompoundVariable(std::make_unique<FuncType>(std::move(param), std::move(ret)), vc, name) {}

  void SetFuncName(const std::string& func_name) { func_name_ = func_name; }

  void SetObj(std::shared_ptr<Variable> obj) { obj_ = obj; }

  void SetCaptureVariables(std::vector<std::shared_ptr<Variable>>&& variables) {
    capture_variables_ = std::move(variables);
  }

  const std::string& GetFuncName() const { return func_name_; }

  std::shared_ptr<Variable> GetObj() const { return obj_; }

  std::vector<std::shared_ptr<Variable>>& GetCaptureVariables() { return capture_variables_; }

  void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) override;

  void DefaultConstruct() override;

  std::unique_ptr<Variable> Clone() override;

  bool Compare(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    if (obj_) {
      return obj_->Compare(other);
    }
    return func_name_ == static_cast<FunctionVariable*>(other.get())->GetFuncName();
  }

  void Assign(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    if (obj_) {
      return obj_->Assign(other);
    }
    auto real_other = static_cast<FunctionVariable*>(other.get());
    func_name_ = real_other->GetFuncName();
    if (other->IsRValue()) {
      capture_variables_ = std::move(real_other->capture_variables_);
    } else {
      capture_variables_.clear();
      for (auto& each : real_other->capture_variables_) {
        capture_variables_.push_back(each->Clone());
      }
    }
  }

  void ChangeTo(ValueCategory vc) {
    vc_ = vc;
    if (obj_ != nullptr) {
      obj_->ChangeTo(vc);
    }
    for (auto& each : capture_variables_) {
      each->ChangeTo(vc);
    }
  }

  void Print(Output& output) override {
    output.OutputFormat("func {} ", GetTypeInfo());
    if (obj_) {
      output.OutPutString("callable");
    } else {
      output.OutputFormat("function {}", func_name_);
    }
  }

 private:
  std::string func_name_;
  std::shared_ptr<Variable> obj_;
  std::vector<std::shared_ptr<Variable>> capture_variables_;
};

inline FunctionVariable* AsFunctionVariable(const std::shared_ptr<Variable>& v) {
  return static_cast<FunctionVariable*>(v.get());
}

inline FunctionVariable* AsFunctionVariable(Variable* v) { return static_cast<FunctionVariable*>(v); }

}  // namespace wamon
