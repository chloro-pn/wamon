#pragma once

#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "wamon/exception.h"
#include "wamon/method_def.h"
#include "wamon/type.h"

namespace wamon {

class StructDef {
 public:
  explicit StructDef(const std::string& name, bool is_trait) : name_(name), is_trait_(is_trait) {}

  void SetStructName(const std::string& name) { name_ = name; }

  const std::string& GetStructName() const { return name_; }

  bool IsTrait() const { return is_trait_; }

  void AddDataMember(const std::string& name, std::unique_ptr<Type>&& type) {
    if (std::find_if(data_members_.begin(), data_members_.end(),
                     [&name](const auto& member) -> bool { return name == member.first; }) != data_members_.end()) {
      throw WamonException("duplicate field {} in type {}", name, name_);
    }
    data_members_.push_back({name, std::move(type)});
  }

  void AddMethods(std::unique_ptr<methods_def>&& ms) {
    for (auto&& each : *ms) {
      if ((IsTrait() && !each->IsDeclaration()) || (!IsTrait() && each->IsDeclaration())) {
        throw WamonException(
            "StructDef.AddMethods error, struct trait only need method declaration and struct must get method declare");
      }
      methods_.emplace_back(std::move(each));
    }
  }

  const std::vector<std::pair<std::string, std::unique_ptr<Type>>>& GetDataMembers() const { return data_members_; }

  const MethodDef* GetMethod(const std::string& method_name) const {
    for (auto& each : methods_) {
      if (each->GetMethodName() == method_name) {
        return each.get();
      }
    }
    return nullptr;
  }

  const MethodDef* GetDestructor() const { return GetMethod("__destructor"); }

  const methods_def& GetMethods() const { return methods_; }

  template <bool throw_if_not_found = true>
  std::unique_ptr<Type> GetDataMemberType(const std::string& field_name) const {
    auto it = std::find_if(data_members_.begin(), data_members_.end(),
                           [&field_name](const auto& member) -> bool { return field_name == member.first; });
    if (it == data_members_.end()) {
      if constexpr (throw_if_not_found) {
        throw WamonException("get data member' type error, field {} not exist", field_name);
      } else {
        return nullptr;
      }
    }
    return it->second->Clone();
  }

  std::vector<std::string> GetDependent(const std::unique_ptr<Type>& type) const {
    // 指针和函数类型直接跳过，因为其包含的类型我们都不需要依赖
    if (IsPtrType(type) || IsFuncType(type)) {
      return {};
    }
    // 数组类型，我们需要获取其包含类型并继续分析
    if (IsListType(type)) {
      auto hold_type = dynamic_cast<ListType*>(type.get())->GetHoldType();
      return GetDependent(hold_type);
    }
    return {type->GetTypeInfo()};
  }

  // 获取结构体定义所依赖的其他结构体的TypeInfo
  // 注意，这里不应该返回所有数据成员涉及的结构体类型，例如ptr(structa)类型并不依赖于structa
  std::set<std::string> GetDependent() const {
    std::set<std::string> ret;
    for (auto& each : data_members_) {
      auto data_member_dependent = GetDependent(each.second);
      for (auto&& tmp : data_member_dependent) {
        ret.insert(std::move(tmp));
      }
    }
    return ret;
  }

 private:
  std::string name_;
  bool is_trait_;
  // 这里需要是有序的，并且按照定义时的顺序
  std::vector<std::pair<std::string, std::unique_ptr<Type>>> data_members_;
  methods_def methods_;
};
}  // namespace wamon
