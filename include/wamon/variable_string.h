#pragma once

#include "wamon/variable.h"

namespace wamon {

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

}  // namespace wamon
