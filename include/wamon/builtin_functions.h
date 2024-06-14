#pragma once

#include <cassert>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "wamon/exception.h"
#include "wamon/type.h"
#include "wamon/variable.h"

namespace wamon {

class Type;
class FuncCallExpr;
class TypeChecker;
class Interpreter;
class PackageUnit;

namespace detail {

template <typename KeyType, typename ValueType>
void MergeMap(std::unordered_map<KeyType, ValueType>& map, std::unordered_map<KeyType, ValueType>&& other) {
  for (auto& each : other) {
    if (map.count(each.first) > 0) {
      throw WamonException("BuiltinFunctions.Merge error, duplicate key {}", each.first);
    }
    map.insert({each.first, std::move(each.second)});
  }
}

}  // namespace detail

class BuiltinFunctions {
 public:
  using HandleType = std::function<std::shared_ptr<Variable>(Interpreter&, std::vector<std::shared_ptr<Variable>>&&)>;
  using CheckType =
      std::function<std::unique_ptr<Type>(const PackageUnit&, const std::vector<std::unique_ptr<Type>>& params_type)>;

  BuiltinFunctions();

  static void RegisterWamonBuiltinFunction(BuiltinFunctions&);

  BuiltinFunctions(BuiltinFunctions&&) = default;

  BuiltinFunctions& operator=(BuiltinFunctions&&) = default;

  std::unique_ptr<Type> TypeCheck(const std::string& name, const PackageUnit& pu,
                                  const std::vector<std::unique_ptr<Type>>& params_type) const;

  bool Find(const std::string& name) const { return builtin_handles_.find(name) != builtin_handles_.end(); }

  HandleType* Get(const std::string& name) const {
    auto it = builtin_handles_.find(name);
    if (it == builtin_handles_.end()) {
      return nullptr;
    }
    return const_cast<HandleType*>(&it->second);
  }

  std::unique_ptr<Type> GetType(const std::string& name) const {
    auto it = builtin_types_.find(name);
    if (it == builtin_types_.end()) {
      return nullptr;
    }
    return it->second->Clone();
  }

  void Register(const std::string& name, CheckType ct, HandleType ht) {
    if (builtin_checks_.find(name) != builtin_checks_.end() || builtin_handles_.find(name) != builtin_handles_.end()) {
      throw WamonException("BuiltinFunctions::Register failed, duplicate name {}", name);
    }
    builtin_checks_[name] = std::move(ct);
    builtin_handles_[name] = std::move(ht);
  }

  void SetTypeForFunction(const std::string& name, std::unique_ptr<Type>&& type) {
    if (builtin_checks_.find(name) == builtin_checks_.end() || builtin_handles_.find(name) == builtin_handles_.end()) {
      throw WamonException("BuiltinFunctions::SetTypeForFunction failed, func {} not exist", name);
    }
    if (!IsFuncType(type)) {
      throw WamonException("BuiltinFunctions::SetTypeForFunction failed, type {} not func type", type->GetTypeInfo());
    }
    builtin_types_[name] = std::move(type);
  }

  void Merge(BuiltinFunctions&& other) {
    detail::MergeMap(builtin_handles_, std::move(other.builtin_handles_));
    detail::MergeMap(builtin_checks_, std::move(other.builtin_checks_));
    detail::MergeMap(builtin_types_, std::move(other.builtin_types_));
  }

 private:
  std::unordered_map<std::string, HandleType> builtin_handles_;
  std::unordered_map<std::string, CheckType> builtin_checks_;
  // 具有固定类型的注册函数需要注册在builtin_types_中。
  std::unordered_map<std::string, std::unique_ptr<Type>> builtin_types_;
};

}  // namespace wamon
