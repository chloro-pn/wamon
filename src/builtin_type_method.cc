#include "wamon/builtin_type_method.h"
#include "wamon/exception.h"

namespace wamon {

std::string concat(const std::string& type, const std::string& method) {
  return type + method;
}

static void throw_params_type_check_error_exception(const std::string& type_info, const std::string& method, const std::string& reason) {
  throw WamonExecption("builtin type method check error , type {} , method {}, reason {}", type_info, method, reason);
}

static void register_builtin_type_method_handle(std::unordered_map<std::string, BuiltInTypeMethod::CheckType>& handles) {
  handles[concat("string", "len")] = [](const std::unique_ptr<Type>& builtin_type, const std::vector<std::unique_ptr<Type>>& params_type) -> std::unique_ptr<Type> {
    if (params_type.empty() == false) {
      throw_params_type_check_error_exception("string", "len", "invalid params count");
    }
    return TypeFactory<int>::Get();
  };

  handles[concat("string", "at")] = [](const std::unique_ptr<Type>& builtin_type, const std::vector<std::unique_ptr<Type>>& params_type) -> std::unique_ptr<Type> {
    if (params_type.size() != 1 || !IsIntType(params_type[0])) {
      throw_params_type_check_error_exception("string", "at", "invalid params count or invalid params type");
    }
    return TypeFactory<char>::Get();
  };

  handles[concat("double", "to_int")] = [](const std::unique_ptr<Type>& builtin_type, const std::vector<std::unique_ptr<Type>>& params_type) -> std::unique_ptr<Type> {
    if (params_type.empty() == false) {
      throw_params_type_check_error_exception("double", "to_int", "invalid params count");
    }
    return TypeFactory<int>::Get();
  };

  handles[concat("list", "size")] = [](const std::unique_ptr<Type>& builtin_type, const std::vector<std::unique_ptr<Type>>& params_type) -> std::unique_ptr<Type> {
    if (params_type.empty() == false) {
      throw_params_type_check_error_exception("list", "size", "invalid params count");
    }
    return TypeFactory<int>::Get();
  };

  handles[concat("list", "at")] = [](const std::unique_ptr<Type>& builtin_type, const std::vector<std::unique_ptr<Type>>& params_type) -> std::unique_ptr<Type> {
    if (params_type.size() != 1 || !IsIntType(params_type[0])) {
      throw_params_type_check_error_exception("list", "at", "invalid params count or invalid params type");
    }
    return GetElementType(builtin_type);
  };
}

BuiltInTypeMethod::BuiltInTypeMethod() {
  register_builtin_type_method_handle(checks_);
}

}