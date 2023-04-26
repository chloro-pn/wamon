#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include "wamon/type.h"
#include "wamon/method_def.h"

namespace wamon {

class StructDef {
 public:
  explicit StructDef(const std::string& name) : name_(name) {

  }

  const std::string& GetStructName() const {
    return name_;
  }

  void AddDataMember(const std::string& name, std::unique_ptr<Type>&& type) {
    data_members_[name] = std::move(type);
  }

  void AddMethods(std::unique_ptr<methods_def>&& ms) {
    for(auto&& each :*ms) {
      methods_.emplace_back(std::move(each));
    }
  }

  const MethodDef* GetMethod(const std::string& method_name) {
    for (auto& each : methods_) {
      if (each->GetMethodName() == method_name) {
        return each.get();
      }
    }
    return nullptr;
  }

  std::unique_ptr<Type> GetDataMemberType(const std::string& field_name) {
    auto it = data_members_.find(field_name);
    if (it == data_members_.end()) {
      throw WamonExecption("get data member' type error, field {} not exist", field_name);
    }
    return it->second->Clone();
  }
  
 private:
  std::string name_;
  std::unordered_map<std::string, std::unique_ptr<Type>> data_members_;
  methods_def methods_;
};
}