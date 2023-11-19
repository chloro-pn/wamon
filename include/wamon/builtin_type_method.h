#pragma once

#include <unordered_map>
#include <string>
#include <functional>
#include <vector>
#include "wamon/type.h"

namespace wamon {

class BuiltInTypeMethod {
 public:
  static BuiltInTypeMethod& Instance() {
    static BuiltInTypeMethod obj;
    return obj;
  }

  using CheckType = std::function<std::unique_ptr<Type>(const std::unique_ptr<Type>& builtin_type, const std::vector<std::unique_ptr<Type>>& params_type)>;

  std::unique_ptr<Type> CheckAndGetReturnType(const std::unique_ptr<Type>& builtintype, const std::string& method_name, const std::vector<std::unique_ptr<Type>>& params_type) {
    std::string handle_id;
    if (IsListType(builtintype)) {
      handle_id = "list"+ method_name;
    } else {
      handle_id = builtintype->GetTypeInfo() + method_name;
    }
    auto handle = checks_.find(handle_id);
    if (handle == checks_.end()) {
      throw WamonExecption("builtinTypeMethod check error, invalid handle id {}", handle_id);
    }
    return handle->second(builtintype, params_type);
  }

 private:
  BuiltInTypeMethod();

  std::unordered_map<std::string, CheckType> checks_;
};

}