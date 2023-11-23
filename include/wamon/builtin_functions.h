#pragma once

#include <cassert>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "wamon/variable.h"

namespace wamon {

class Type;
class FuncCallExpr;
class TypeChecker;

class BuiltinFunctions {
 public:
  using HandleType = std::function<std::shared_ptr<Variable>(std::vector<std::shared_ptr<Variable>>&&)>;
  using CheckType = std::function<std::unique_ptr<Type>(const std::vector<std::unique_ptr<Type>>& params_type)>;

  BuiltinFunctions();

  static BuiltinFunctions& Instance() {
    static BuiltinFunctions obj;
    return obj;
  }

  std::unique_ptr<Type> TypeCheck(const std::string& name, const std::vector<std::unique_ptr<Type>>& params_type);

  bool Find(const std::string& name) { return builtin_handles_.find(name) != builtin_handles_.end(); }

  HandleType* Get(const std::string& name) {
    auto it = builtin_handles_.find(name);
    if (it == builtin_handles_.end()) {
      return nullptr;
    }
    return &builtin_handles_[name];
  }

  std::string Register(const std::string& name, CheckType ct, HandleType ht) {
    if (builtin_checks_.find(name) != builtin_checks_.end() || builtin_handles_.find(name) != builtin_handles_.end()) {
      return "register failed, duplicate name " + name;
    }
    builtin_checks_[name] = std::move(ct);
    builtin_handles_[name] = std::move(ht);
    return "succ";
  }

 private:
  std::unordered_map<std::string, HandleType> builtin_handles_;
  std::unordered_map<std::string, CheckType> builtin_checks_;
};

}  // namespace wamon