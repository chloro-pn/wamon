#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "wamon/type.h"
#include "wamon/variable.h"

namespace wamon {

class PackageUnit;

class InnerTypeMethod {
 public:
  static InnerTypeMethod& Instance() {
    static InnerTypeMethod obj;
    return obj;
  }

  using CheckType = std::function<std::unique_ptr<Type>(const std::unique_ptr<Type>& builtin_type,
                                                        const std::vector<std::unique_ptr<Type>>& params_type)>;
  using HandleType = std::function<std::shared_ptr<Variable>(
      std::shared_ptr<Variable>& obj, std::vector<std::shared_ptr<Variable>>&&, const PackageUnit&)>;

  std::string GetHandleId(const std::unique_ptr<Type>& builtintype, const std::string& method_name) const {
    std::string handle_id;
    if (IsListType(builtintype)) {
      handle_id = "list" + method_name;
    } else {
      handle_id = builtintype->GetTypeInfo() + method_name;
    }
    return handle_id;
  }

  std::unique_ptr<Type> CheckAndGetReturnType(const std::unique_ptr<Type>& builtintype, const std::string& method_name,
                                              const std::vector<std::unique_ptr<Type>>& params_type) const {
    std::string handle_id = GetHandleId(builtintype, method_name);
    auto handle = checks_.find(handle_id);
    if (handle == checks_.end()) {
      throw WamonExecption("builtinTypeMethod check error, invalid handle id {}", handle_id);
    }
    return handle->second(builtintype, params_type);
  }

  HandleType& Get(std::shared_ptr<Variable>& obj, const std::string& method_name) const {
    assert(!IsStructType(obj->GetType()));
    auto handle_id = GetHandleId(obj->GetType(), method_name);
    auto it = handles_.find(handle_id);
    assert(it != handles_.end());
    return const_cast<HandleType&>(it->second);
  }

 private:
  InnerTypeMethod();
  std::unordered_map<std::string, CheckType> checks_;
  std::unordered_map<std::string, HandleType> handles_;
};

}  // namespace wamon
