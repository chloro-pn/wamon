#pragma once

#include <string>
#include <vector>

#include "nlohmann/json.hpp"
#include "wamon/exception.h"
#include "wamon/type.h"

namespace wamon {

class StructDef;
class EnumDef;

/*
 * Variable为运行时变量类型的基类，每个Variable一定包含一个type成员标识其类型
 * Variable本身不包含名字，其名字需要被使用者持有
 * 每个Variable都具有值型别(like c++)，左值或者右值
 */
class Variable {
 public:
  enum class ValueCategory {
    LValue,
    RValue,
  };

  explicit Variable(std::unique_ptr<Type>&& t, ValueCategory vc) : type_(std::move(t)), vc_(vc) {}

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

 protected:
  ValueCategory vc_;
};

std::shared_ptr<Variable> VariableMove(const std::shared_ptr<Variable>& v);

std::shared_ptr<Variable> VariableMoveOrCopy(const std::shared_ptr<Variable>& v);

class PackageUnit;
class Interpreter;
std::shared_ptr<Variable> VariableFactory(const std::unique_ptr<Type>& type, Variable::ValueCategory vc,
                                          Interpreter* ip = nullptr);

std::shared_ptr<Variable> VariableFactory(const std::unique_ptr<Type>& type, Variable::ValueCategory vc,
                                          Interpreter& ip);

class Interpreter;

class StructVariable : public Variable {
 public:
  StructVariable(const StructDef* sd, ValueCategory vc, Interpreter& i);

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
  EnumVariable(const EnumDef* def, ValueCategory vc);

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
  CompoundVariable(std::unique_ptr<Type>&& t, ValueCategory vc) : Variable(std::move(t), vc) {}
};

class PointerVariable : public CompoundVariable {
 public:
  PointerVariable(std::unique_ptr<Type>&& hold_type, ValueCategory vc)
      : CompoundVariable(std::make_unique<PointerType>(std::move(hold_type)), vc) {}

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

class FunctionVariable : public CompoundVariable {
 public:
  struct capture_item {
    bool is_ref;
    std::shared_ptr<Variable> v;
  };

  FunctionVariable(std::vector<std::unique_ptr<Type>>&& param, std::unique_ptr<Type>&& ret, ValueCategory vc)
      : CompoundVariable(std::make_unique<FuncType>(std::move(param), std::move(ret)), vc) {}

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
          tmp->ChangeTo(vc_);
          capture_variables_.push_back({each.is_ref, tmp});
        }
      }
    }
  }

  // ref捕获的变量和copy、move捕获的变量最大的区别在于，ref捕获的变量总是LValue的，而copy、move捕获的变量跟随FunctionVariable变动
  void ChangeTo(ValueCategory vc) override {
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

}  // namespace wamon
