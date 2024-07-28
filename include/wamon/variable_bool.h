#pragma once

#include "wamon/variable.h"

namespace wamon {

class BoolVariable : public Variable {
 public:
  BoolVariable(bool v, ValueCategory vc) : Variable(TypeFactory<bool>::Get(), vc), value_(v) {}

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
    return std::make_shared<BoolVariable>(GetValue(), ValueCategory::RValue);
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

inline BoolVariable* AsBoolVariable(const std::shared_ptr<Variable>& v) { return static_cast<BoolVariable*>(v.get()); }

}  // namespace wamon
