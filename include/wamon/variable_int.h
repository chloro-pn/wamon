#pragma once

#include "wamon/variable.h"

namespace wamon {

class IntVariable : public Variable {
 public:
  IntVariable(int v, ValueCategory vc) : Variable(TypeFactory<int>::Get(), vc), value_(v) {}

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
    return std::make_shared<IntVariable>(GetValue(), ValueCategory::RValue);
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

}  // namespace wamon
