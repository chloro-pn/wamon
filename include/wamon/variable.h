#pragma once

#include <concepts>
#include <string>
#include <string_view>
#include <vector>

#include "nlohmann/json.hpp"
#include "wamon/exception.h"
#include "wamon/type.h"

namespace wamon {

class StructDef;
class EnumDef;

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

  // Clone函数返回值的 ValueCategory == RValue，name == ""
  virtual std::shared_ptr<Variable> Clone() = 0;

  virtual bool Compare(const std::shared_ptr<Variable>& other) = 0;

  virtual void Assign(const std::shared_ptr<Variable>& other) = 0;

  virtual nlohmann::json Print() = 0;

  virtual ~Variable() = default;

  virtual std::string GetTypeInfo() const { return type_->GetTypeInfo(); }

  virtual std::unique_ptr<Type> GetType() const { return type_->Clone(); }

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

std::shared_ptr<Variable> VariableMove(const std::shared_ptr<Variable>& v);

std::shared_ptr<Variable> VariableMoveOrCopy(const std::shared_ptr<Variable>& v);

class PackageUnit;
class Interpreter;
std::shared_ptr<Variable> VariableFactory(const std::unique_ptr<Type>& type, Variable::ValueCategory vc,
                                          const std::string& name, Interpreter* ip = nullptr);

std::shared_ptr<Variable> VariableFactory(const std::unique_ptr<Type>& type, Variable::ValueCategory vc,
                                          const std::string& name, Interpreter& ip);

std::shared_ptr<Variable> GetVoidVariable();

class VoidVariable : public Variable {
 public:
  explicit VoidVariable(std::unique_ptr<Type> type = TypeFactory<void>::Get())
      : Variable(std::move(type), ValueCategory::RValue) {}

  void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) override {
    throw WamonException("VoidVariable should not call ConstructByFields method");
  }

  void DefaultConstruct() override { throw WamonException("VoidVariable should not call DefaultConstruct method"); }

  bool Compare(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    return true;
  }

  void Assign(const std::shared_ptr<Variable>& other) override {
    throw WamonException("VoidVariable.Assign should not be called");
  }

  nlohmann::json Print() override { throw WamonException("VoidVariable.Print should not be called"); }

  std::shared_ptr<Variable> Clone() override { return std::make_shared<VoidVariable>(); }
};

class TypeVariable : public VoidVariable {
 public:
  TypeVariable(std::unique_ptr<Type>&& type) : VoidVariable(std::move(type)) {}

  bool Compare(const std::shared_ptr<Variable>& other) override { return GetTypeInfo() == other->GetTypeInfo(); }

  std::shared_ptr<Variable> Clone() override { return std::make_shared<TypeVariable>(GetType()->Clone()); }
};

inline TypeVariable* AsTypeVariable(const std::shared_ptr<Variable>& v) { return static_cast<TypeVariable*>(v.get()); }

class StringVariable : public Variable {
 public:
  StringVariable(const std::string& v, ValueCategory vc, const std::string& name)
      : Variable(TypeFactory<std::string>::Get(), vc, name), value_(v) {}

  const std::string& GetValue() const { return value_; }

  std::string& GetValue() { return value_; }

  void SetValue(std::string_view v) { value_ = v; }

  void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) override {
    if (fields.size() != 1) {
      throw WamonException("StringVariable's ConstructByFields method error : fields.size() == {}", fields.size());
    }
    if (fields[0]->GetTypeInfo() != GetTypeInfo()) {
      throw WamonException("StringVariable's ConstructByFields method error, type dismatch : {} != {}",
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

  std::shared_ptr<Variable> Clone() override {
    std::shared_ptr<StringVariable> ret;
    if (IsRValue()) {
      ret = std::make_shared<StringVariable>(std::move(value_), ValueCategory::RValue, "");
    } else {
      ret = std::make_shared<StringVariable>(GetValue(), ValueCategory::RValue, "");
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

  nlohmann::json Print() override { return nlohmann::json(value_); }

 private:
  std::string value_;
};

inline StringVariable* AsStringVariable(const std::shared_ptr<Variable>& v) {
  return static_cast<StringVariable*>(v.get());
}

class BoolVariable : public Variable {
 public:
  BoolVariable(bool v, ValueCategory vc, const std::string& name)
      : Variable(TypeFactory<bool>::Get(), vc, name), value_(v) {}

  bool GetValue() const { return value_; }

  void SetValue(bool b) { value_ = b; }

  void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) override {
    if (fields.size() != 1) {
      throw WamonException("BoolVariable's ConstructByFields method error : fields.size() == {}", fields.size());
    }
    if (fields[0]->GetTypeInfo() != GetTypeInfo()) {
      throw WamonException("BoolVariable's ConstructByFields method error, type dismatch : {} != {}",
                           fields[0]->GetTypeInfo(), GetTypeInfo());
    }
    BoolVariable* ptr = static_cast<BoolVariable*>(fields[0].get());
    value_ = ptr->GetValue();
  }

  void DefaultConstruct() override { value_ = true; }

  std::shared_ptr<Variable> Clone() override {
    return std::make_shared<BoolVariable>(GetValue(), ValueCategory::RValue, "");
  }

  bool Compare(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    return value_ == static_cast<BoolVariable*>(other.get())->value_;
  }

  void Assign(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    value_ = static_cast<BoolVariable*>(other.get())->value_;
  }

  nlohmann::json Print() override { return nlohmann::json(value_); }

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
      throw WamonException("IntVariable's ConstructByFields method error : fields.size() == {}", fields.size());
    }
    if (fields[0]->GetTypeInfo() != GetTypeInfo()) {
      throw WamonException("IntVariable's ConstructByFields method error, type dismatch : {} != {}",
                           fields[0]->GetTypeInfo(), GetTypeInfo());
    }
    IntVariable* ptr = static_cast<IntVariable*>(fields[0].get());
    value_ = ptr->GetValue();
  }

  void DefaultConstruct() override { value_ = 0; }

  std::shared_ptr<Variable> Clone() override {
    return std::make_shared<IntVariable>(GetValue(), ValueCategory::RValue, "");
  }

  bool Compare(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    return value_ == static_cast<IntVariable*>(other.get())->value_;
  }

  void Assign(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    value_ = static_cast<IntVariable*>(other.get())->value_;
  }

  nlohmann::json Print() override { return nlohmann::json(value_); }

 private:
  int value_;
};

inline IntVariable* AsIntVariable(const std::shared_ptr<Variable>& v) { return static_cast<IntVariable*>(v.get()); }

class DoubleVariable : public Variable {
 public:
  DoubleVariable(double v, ValueCategory vc, const std::string& name)
      : Variable(TypeFactory<double>::Get(), vc, name), value_(v) {}

  double GetValue() const { return value_; }

  void SetValue(double d) { value_ = d; }

  void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) override {
    if (fields.size() != 1) {
      throw WamonException("DoubleVariable's ConstructByFields method error : fields.size() == {}", fields.size());
    }
    if (fields[0]->GetTypeInfo() != GetTypeInfo()) {
      throw WamonException("DoubleVariable's ConstructByFields method error, type dismatch : {} != {}",
                           fields[0]->GetTypeInfo(), GetTypeInfo());
    }
    DoubleVariable* ptr = static_cast<DoubleVariable*>(fields[0].get());
    value_ = ptr->GetValue();
  }

  void DefaultConstruct() override { value_ = 0.0; }

  std::shared_ptr<Variable> Clone() override {
    return std::make_shared<DoubleVariable>(GetValue(), ValueCategory::RValue, "");
  }

  bool Compare(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    return value_ == static_cast<DoubleVariable*>(other.get())->value_;
  }

  void Assign(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    value_ = static_cast<DoubleVariable*>(other.get())->value_;
  }

  nlohmann::json Print() override { return nlohmann::json(value_); }

 private:
  double value_;
};

inline DoubleVariable* AsDoubleVariable(const std::shared_ptr<Variable>& v) {
  return static_cast<DoubleVariable*>(v.get());
}

inline void byte_to_string(unsigned char c, char (&buf)[4]) {
  buf[0] = '0';
  buf[1] = 'X';
  int h = static_cast<int>(c) / 16;
  int l = static_cast<int>(c) - h * 16;
  if (h >= 0 && h <= 9) {
    buf[2] = h + '0';
  } else {
    buf[2] = h - 10 + 'A';
  }
  if (l >= 0 && l <= 9) {
    buf[3] = l + '0';
  } else {
    buf[3] = l - 10 + 'A';
  }
}

class ByteVariable : public Variable {
 public:
  ByteVariable(unsigned char v, ValueCategory vc, const std::string& name)
      : Variable(TypeFactory<unsigned char>::Get(), vc, name), value_(v) {}

  unsigned char GetValue() const { return value_; }

  void SetValue(unsigned char c) { value_ = c; }

  void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) override {
    if (fields.size() != 1) {
      throw WamonException("ByteVariable's ConstructByFields method error : fields.size() == {}", fields.size());
    }
    if (fields[0]->GetTypeInfo() != GetTypeInfo()) {
      throw WamonException("ByteVariable's ConstructByFields method error, type dismatch : {} != {}",
                           fields[0]->GetTypeInfo(), GetTypeInfo());
    }
    ByteVariable* ptr = static_cast<ByteVariable*>(fields[0].get());
    value_ = ptr->GetValue();
  }

  void DefaultConstruct() override { value_ = 0; }

  std::shared_ptr<Variable> Clone() override {
    return std::make_shared<ByteVariable>(GetValue(), ValueCategory::RValue, "");
  }

  bool Compare(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    return value_ == static_cast<ByteVariable*>(other.get())->value_;
  }

  void Assign(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    value_ = static_cast<ByteVariable*>(other.get())->value_;
  }

  nlohmann::json Print() override {
    char buf[4];
    byte_to_string(value_, buf);
    return nlohmann::json(std::string(buf, 4));
  }

 private:
  unsigned char value_;
};

inline ByteVariable* AsByteVariable(const std::shared_ptr<Variable>& v) { return static_cast<ByteVariable*>(v.get()); }

class Interpreter;

class StructVariable : public Variable {
 public:
  StructVariable(const StructDef* sd, ValueCategory vc, Interpreter& i, const std::string& name);

  std::string GetTypeInfo() const override;

  std::unique_ptr<Type> GetType() const override;

  const StructDef* GetStructDef() const { return def_; }

  std::shared_ptr<Variable> GetDataMemberByName(const std::string& name);

  void AddDataMemberByName(const std::string& name, std::shared_ptr<Variable> data);

  void UpdateDataMemberByName(const std::string& name, std::shared_ptr<Variable> data);

  void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) override;

  void DefaultConstruct() override;

  std::shared_ptr<Variable> Clone() override;

  ~StructVariable();

  void set_trait_def(const StructDef* def) { trait_def_ = def; }

 private:
  static void trait_construct(StructVariable* lv, StructVariable* rv);

  static bool trait_compare(StructVariable* lv, StructVariable* rv);

  static void trait_assign(StructVariable* lv, StructVariable* rv);

  // 目前仅支持相同类型的trait间的比较和赋值
  bool Compare(const std::shared_ptr<Variable>& other) override;

  void Assign(const std::shared_ptr<Variable>& other) override;

  void ChangeTo(ValueCategory vc) override;

  nlohmann::json Print() override;

 private:
  const StructDef* def_;
  const StructDef* trait_def_;
  Interpreter& ip_;
  struct member {
    std::string name;
    std::shared_ptr<Variable> data;
  };
  std::vector<member> data_members_;
};

inline StructVariable* AsStructVariable(const std::shared_ptr<Variable>& v) {
  return static_cast<StructVariable*>(v.get());
}

class EnumVariable : public Variable {
 public:
  EnumVariable(const EnumDef* def, ValueCategory vc, const std::string& name);

  void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) override;

  void DefaultConstruct() override;

  // Clone函数返回值的 ValueCategory == RValue，name == ""
  std::shared_ptr<Variable> Clone() override;

  bool Compare(const std::shared_ptr<Variable>& other) override;

  void Assign(const std::shared_ptr<Variable>& other) override;

  nlohmann::json Print() override;

  void SetEnumItem(const std::string& enum_item) { enum_item_ = enum_item; }

  const std::string& GetEnumItem() const { return enum_item_; }

 private:
  const EnumDef* def_;
  std::string enum_item_;
};

inline EnumVariable* AsEnumVariable(const std::shared_ptr<Variable>& v) { return static_cast<EnumVariable*>(v.get()); }

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

  std::unique_ptr<Type> GetHoldType() const { return ::wamon::GetHoldType(GetType()); }

  void SetHoldVariable(std::shared_ptr<Variable> v) { obj_ = v; }

  void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) override;

  void DefaultConstruct() override;

  std::shared_ptr<Variable> Clone() override;

  bool Compare(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    return obj_.lock() == static_cast<PointerVariable*>(other.get())->obj_.lock();
  }

  void Assign(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    obj_ = static_cast<PointerVariable*>(other.get())->obj_;
  }

  nlohmann::json Print() override {
    auto v = obj_.lock();
    if (v) {
      nlohmann::json ptr_to;
      ptr_to["point_to"] = v->Print();
      return ptr_to;
    } else {
      return nlohmann::json("nullptr");
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
  ListVariable(std::unique_ptr<Type>&& element_type, ValueCategory vc, Interpreter& ip, const std::string& name)
      : CompoundVariable(std::make_unique<ListType>(element_type->Clone()), vc, name),
        element_type_(std::move(element_type)),
        ip_(ip) {}

  void PushBack(std::shared_ptr<Variable> element);

  void PopBack();

  size_t Size() const { return elements_.size(); }

  void Resize(size_t new_size) {
    size_t old_size = Size();
    if (new_size == old_size) {
      return;
    } else if (new_size < old_size) {
      elements_.resize(new_size);
    } else {
      for (size_t i = 0; i < (new_size - old_size); ++i) {
        auto v = VariableFactory(element_type_, vc_, "", ip_);
        v->DefaultConstruct();
        elements_.push_back(std::move(v));
      }
    }
  }

  void Clear() { elements_.clear(); }

  void Erase(size_t index) {
    if (index >= elements_.size()) {
      throw WamonException("List.Erase error, index out of range : {} >= {}", index, elements_.size());
    }
    auto it = elements_.begin();
    std::advance(it, index);
    elements_.erase(it);
  }

  void Insert(size_t index, std::shared_ptr<Variable> v) {
    if (index > Size()) {
      throw WamonException("List.Insert error, index = {}, size = {}", index, Size());
    }

    std::shared_ptr<Variable> tmp;
    if (v->IsRValue()) {
      tmp = v;
    } else {
      tmp = v->Clone();
    }
    tmp->ChangeTo(vc_);
    auto it = elements_.begin();
    std::advance(it, index);
    elements_.insert(it, std::move(tmp));
  }

  std::shared_ptr<Variable> at(size_t i) {
    if (i >= Size()) {
      throw WamonException("ListVariable.at error, index {} out of range", i);
    }
    return elements_[i];
  }

  std::string get_string_only_for_byte_list() {
    if (!IsByteType(element_type_)) {
      throw WamonException("ListVariable.get_string_only_for_byte_list can only be called by List(byte) type");
    }
    std::string ret;
    for (auto& each : elements_) {
      ret.push_back(char(AsByteVariable(each)->GetValue()));
    }
    return ret;
  }

  void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) override;

  void DefaultConstruct() override;

  std::shared_ptr<Variable> Clone() override;

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
      // PushBack函数会处理值型别
      PushBack(list_v->elements_[i]);
    }
  }

  void ChangeTo(ValueCategory vc) override {
    vc_ = vc;
    for (auto& each : elements_) {
      each->ChangeTo(vc);
    }
  }

  nlohmann::json Print() override {
    nlohmann::json list;
    for (size_t i = 0; i < elements_.size(); ++i) {
      list.push_back(elements_[i]->Print());
    }
    return list;
  }

 private:
  std::unique_ptr<Type> element_type_;
  std::vector<std::shared_ptr<Variable>> elements_;
  Interpreter& ip_;
};

inline ListVariable* AsListVariable(const std::shared_ptr<Variable>& v) { return static_cast<ListVariable*>(v.get()); }

class FunctionVariable : public CompoundVariable {
 public:
  struct capture_item {
    bool is_ref;
    std::shared_ptr<Variable> v;
  };

  FunctionVariable(std::vector<std::unique_ptr<Type>>&& param, std::unique_ptr<Type>&& ret, ValueCategory vc,
                   const std::string& name)
      : CompoundVariable(std::make_unique<FuncType>(std::move(param), std::move(ret)), vc, name) {}

  void SetFuncName(const std::string& func_name) { func_name_ = func_name; }

  void SetObj(std::shared_ptr<Variable> obj) { obj_ = obj; }

  void SetCaptureVariables(std::vector<capture_item>&& variables) { capture_variables_ = std::move(variables); }

  const std::string& GetFuncName() const { return func_name_; }

  std::shared_ptr<Variable> GetObj() const { return obj_; }

  std::vector<capture_item>& GetCaptureVariables() { return capture_variables_; }

  void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) override;

  void DefaultConstruct() override;

  std::shared_ptr<Variable> Clone() override;

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
        if (each.is_ref == true) {
          assert(!each.v->IsRValue());
          capture_variables_.push_back(each);
        } else {
          auto tmp = each.v->Clone();
          tmp->SetName(each.v->GetName());
          tmp->ChangeTo(vc_);
          capture_variables_.push_back({each.is_ref, tmp});
        }
      }
    }
  }

  // ref捕获的变量和copy、move捕获的变量最大的区别在于，ref捕获的变量总是LValue的，而copy、move捕获的变量跟随FunctionVariable变动
  void ChangeTo(ValueCategory vc) {
    vc_ = vc;
    if (obj_ != nullptr) {
      obj_->ChangeTo(vc);
    }
    for (auto& each : capture_variables_) {
      if (each.is_ref == false) {
        each.v->ChangeTo(vc_);
      }
    }
  }

  nlohmann::json Print() override {
    nlohmann::json j;
    j["function_type"] = GetTypeInfo();
    if (obj_) {
      j["functor"] = obj_->Print();
    } else {
      j["function"] = func_name_;
    }
    for (auto& each : capture_variables_) {
      j["capture_variables"].push_back(each.v->Print());
    }
    return j;
  }

 private:
  std::string func_name_;
  std::shared_ptr<Variable> obj_;
  std::vector<capture_item> capture_variables_;
};

inline FunctionVariable* AsFunctionVariable(const std::shared_ptr<Variable>& v) {
  return static_cast<FunctionVariable*>(v.get());
}

/* ********************************************************************
 * API : VarAs
 *
 * ********************************************************************/
template <typename T>
T VarAs(const std::shared_ptr<Variable>& v) {
  if constexpr (std::is_same_v<int, T>) {
    assert(IsSameType(TypeFactory<int>::Get(), v->GetType()));
    return AsIntVariable(v)->GetValue();
  }
  if constexpr (std::is_same_v<double, T>) {
    assert(IsSameType(TypeFactory<double>::Get(), v->GetType()));
    return AsDoubleVariable(v)->GetValue();
  }
  if constexpr (std::is_same_v<unsigned char, T>) {
    assert(IsSameType(TypeFactory<unsigned char>::Get(), v->GetType()));
    return AsByteVariable(v)->GetValue();
  }
  if constexpr (std::is_same_v<bool, T>) {
    assert(IsSameType(TypeFactory<bool>::Get(), v->GetType()));
    return AsBoolVariable(v)->GetValue();
  }
  if constexpr (std::is_same_v<std::string, T>) {
    assert(IsSameType(TypeFactory<std::string>::Get(), v->GetType()));
    return AsStringVariable(v)->GetValue();
  }
  throw WamonException("VarAs error, not support type {} now", v->GetTypeInfo());
  WAMON_UNREACHABLE;
  return *static_cast<T*>(nullptr);
}

/* ********************************************************************
 * API : ToVar
 *
 * ********************************************************************/
#define WAMON_TO_VAR(basic_type, transform_type)                                                     \
  if constexpr (std::is_same_v<type, basic_type>) {                                                  \
    auto ret = VariableFactory(TypeFactory<basic_type>::Get(), Variable::ValueCategory::LValue, ""); \
    As##transform_type##Variable(ret)->SetValue(std::forward<T>(v));                                 \
    return ret;                                                                                      \
  }

inline std::shared_ptr<Variable> ToVar(std::string_view strv) {
  auto ret = VariableFactory(TypeFactory<std::string>::Get(), Variable::ValueCategory::LValue, "");
  AsStringVariable(ret)->SetValue(strv);
  return ret;
}

template <size_t n>
std::shared_ptr<Variable> ToVar(char (&cstr)[n]) {
  return ToVar(std::string_view(cstr));
}

template <typename T>
concept WAMON_SUPPORT_TOVAR = std::same_as<T, int> || std::same_as<T, double> || std::same_as<T, unsigned char> ||
    std::same_as<T, bool> || std::same_as<T, std::string>;

template <typename T>
requires WAMON_SUPPORT_TOVAR<std::remove_cvref_t<T>> std::shared_ptr<Variable> ToVar(T&& v) {
  using type = std::remove_cvref_t<T>;
  WAMON_TO_VAR(int, Int)
  WAMON_TO_VAR(double, Double)
  WAMON_TO_VAR(unsigned char, Byte)
  WAMON_TO_VAR(bool, Bool)
  WAMON_TO_VAR(std::string, String)

  WAMON_UNREACHABLE;
  return nullptr;
}

inline std::shared_ptr<Variable> ToVar(std::shared_ptr<Variable> v) { return v; }

}  // namespace wamon
