#pragma once

#include <string>
#include <vector>
#include <utility>
#include <memory>
#include <algorithm>

#include "wamon/type.h"
#include "wamon/method_def.h"
#include "wamon/exception.h"

namespace wamon {

class StructDef {
 public:
  explicit StructDef(const std::string& name) : name_(name) {

  }

  const std::string& GetStructName() const {
    return name_;
  }

  void AddDataMember(const std::string& name, std::unique_ptr<Type>&& type) {
    if (std::find_if(data_members_.begin(), data_members_.end(), [&name](const auto& member) -> bool {
      return name == member.first;
    }) != data_members_.end()) {
      throw WamonExecption("duplicate field {} in type {}", name, name_);
    }
    data_members_.push_back({name, std::move(type)});
  }

  void AddMethods(std::unique_ptr<methods_def>&& ms) {
    for(auto&& each :*ms) {
      methods_.emplace_back(std::move(each));
    }
  }

  const std::vector<std::pair<std::string, std::unique_ptr<Type>>>& GetDataMembers() const {
    return data_members_;
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
    auto it = std::find_if(data_members_.begin(), data_members_.end(), [&field_name](const auto& member) -> bool {
      return field_name == member.first;
    });
    if (it == data_members_.end()) {
      throw WamonExecption("get data member' type error, field {} not exist", field_name);
    }
    return it->second->Clone();
  }
  
 private:
  std::string name_;
  // 这里需要是有序的，并且按照定义时的顺序
  std::vector<std::pair<std::string, std::unique_ptr<Type>>> data_members_;
  methods_def methods_;
};
}