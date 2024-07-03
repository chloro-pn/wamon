#pragma once

#include "wamon/variable.h"

namespace wamon {

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

std::shared_ptr<Variable> GetVoidVariable();

inline TypeVariable* AsTypeVariable(const std::shared_ptr<Variable>& v) { return static_cast<TypeVariable*>(v.get()); }

}  // namespace wamon
