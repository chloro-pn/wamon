#include "wamon/inner_type_method.h"

#include "wamon/exception.h"

namespace wamon {

std::string concat(const std::string& type, const std::string& method) { return type + method; }

static void throw_params_type_check_error_exception(const std::string& type_info, const std::string& method,
                                                    const std::string& reason) {
  throw WamonExecption("builtin type method check error , type {} , method {}, reason {}", type_info, method, reason);
}

static void register_builtin_type_method_check(std::unordered_map<std::string, InnerTypeMethod::CheckType>& handles) {
  handles[concat("string", "len")] =
      [](const std::unique_ptr<Type>& builtin_type,
         const std::vector<std::unique_ptr<Type>>& params_type) -> std::unique_ptr<Type> {
    if (params_type.empty() == false) {
      throw_params_type_check_error_exception("string", "len", "invalid params count");
    }
    return TypeFactory<int>::Get();
  };

  handles[concat("string", "at")] = [](const std::unique_ptr<Type>& builtin_type,
                                       const std::vector<std::unique_ptr<Type>>& params_type) -> std::unique_ptr<Type> {
    if (params_type.size() != 1 || !IsIntType(params_type[0])) {
      throw_params_type_check_error_exception("string", "at", "invalid params count or invalid params type");
    }
    return TypeFactory<unsigned char>::Get();
  };

  handles[concat("double", "to_int")] =
      [](const std::unique_ptr<Type>& builtin_type,
         const std::vector<std::unique_ptr<Type>>& params_type) -> std::unique_ptr<Type> {
    if (params_type.empty() == false) {
      throw_params_type_check_error_exception("double", "to_int", "invalid params count");
    }
    return TypeFactory<int>::Get();
  };

  handles[concat("list", "size")] = [](const std::unique_ptr<Type>& builtin_type,
                                       const std::vector<std::unique_ptr<Type>>& params_type) -> std::unique_ptr<Type> {
    if (params_type.empty() == false) {
      throw_params_type_check_error_exception("list", "size", "invalid params count");
    }
    return TypeFactory<int>::Get();
  };

  handles[concat("list", "at")] = [](const std::unique_ptr<Type>& builtin_type,
                                     const std::vector<std::unique_ptr<Type>>& params_type) -> std::unique_ptr<Type> {
    if (params_type.size() != 1 || !IsIntType(params_type[0])) {
      throw_params_type_check_error_exception("list", "at", "invalid params count or invalid params type");
    }
    return GetElementType(builtin_type);
  };
}

static void register_builtin_type_method_handle(std::unordered_map<std::string, InnerTypeMethod::HandleType>& handles) {
  handles[concat("string", "len")] = [](std::shared_ptr<Variable>& obj,
                                        std::vector<std::shared_ptr<Variable>>&& params) -> std::shared_ptr<Variable> {
    assert(params.empty());
    int ret = AsStringVariable(obj)->GetValue().size();
    return std::make_shared<IntVariable>(ret, Variable::ValueCategory::RValue, "");
  };

  handles[concat("string", "at")] = [](std::shared_ptr<Variable>& obj,
                                       std::vector<std::shared_ptr<Variable>>&& params) -> std::shared_ptr<Variable> {
    assert(params.size() == 1);
    const std::string& v = AsStringVariable(obj)->GetValue();
    int index = AsIntVariable(params[0])->GetValue();
    if (static_cast<size_t>(index) >= v.size()) {
      throw WamonExecption("string.at error, index out of range : {} >= {}", index, v.size());
    }
    return std::make_shared<ByteVariable>(v[index], Variable::ValueCategory::RValue, "");
  };

  handles[concat("double", "to_int")] =
      [](std::shared_ptr<Variable>& obj, std::vector<std::shared_ptr<Variable>>&& params) -> std::shared_ptr<Variable> {
    assert(params.empty());
    double ret = AsDoubleVariable(obj)->GetValue();
    return std::make_shared<IntVariable>(static_cast<int>(ret), Variable::ValueCategory::RValue, "");
  };

  handles[concat("list", "size")] = [](std::shared_ptr<Variable>& obj,
                                       std::vector<std::shared_ptr<Variable>>&& params) -> std::shared_ptr<Variable> {
    assert(params.empty());
    int ret = AsListVariable(obj)->Size();
    return std::make_shared<IntVariable>(ret, Variable::ValueCategory::RValue, "");
  };

  handles[concat("list", "at")] = [](std::shared_ptr<Variable>& obj,
                                     std::vector<std::shared_ptr<Variable>>&& params) -> std::shared_ptr<Variable> {
    assert(params.size() == 1);
    int index = AsIntVariable(params[0])->GetValue();
    return AsListVariable(obj)->at(index);
  };
}

InnerTypeMethod::InnerTypeMethod() {
  register_builtin_type_method_check(checks_);
  register_builtin_type_method_handle(handles_);
}

}  // namespace wamon
