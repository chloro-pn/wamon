#pragma once

#include "wamon/variable.h"

namespace wamon {

class DoubleVariable : public Variable {
 public:
  DoubleVariable(double v, ValueCategory vc) : Variable(TypeFactory<double>::Get(), vc), value_(v) {}

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
    return std::make_shared<DoubleVariable>(GetValue(), ValueCategory::RValue);
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

}  // namespace wamon
