#include "wamon/inner_type_method.h"

#include <wamon/interpreter.h>

#include "wamon/exception.h"
#include "wamon/variable_list.h"

namespace wamon {

std::string concat(const std::string& type, const std::string& method) { return type + method; }

static void register_builtin_type_method_check(std::unordered_map<std::string, InnerTypeMethod::CheckType>& handles) {
  handles[concat("string", "len")] =
      [](const std::unique_ptr<Type>& builtin_type,
         const std::vector<std::unique_ptr<Type>>& params_type) -> std::unique_ptr<Type> {
    if (params_type.empty() == false) {
      throw WamonException("string.len error, params.size() == {}", params_type.size());
    }
    return TypeFactory<int>::Get();
  };

  handles[concat("string", "at")] = [](const std::unique_ptr<Type>& builtin_type,
                                       const std::vector<std::unique_ptr<Type>>& params_type) -> std::unique_ptr<Type> {
    if (params_type.size() != 1) {
      throw WamonException("string.at error, params.size() == {}", params_type.size());
    }
    if (!IsIntType(params_type[0])) {
      throw WamonException("string.at error, param's type == {}", params_type[0]->GetTypeInfo());
    }
    return TypeFactory<unsigned char>::Get();
  };

  handles[concat("string", "append")] =
      [](const std::unique_ptr<Type>& builtin_type,
         const std::vector<std::unique_ptr<Type>>& params_type) -> std::unique_ptr<Type> {
    if (params_type.size() != 1) {
      throw WamonException("string.append error, params.size() == {}", params_type.size());
    }
    if (!IsStringType(params_type[0]) && !IsByteType(params_type[0])) {
      throw WamonException("string.append error, param's type == {}", params_type[0]->GetTypeInfo());
    }
    return TypeFactory<void>::Get();
  };

  handles[concat("list", "size")] = [](const std::unique_ptr<Type>& builtin_type,
                                       const std::vector<std::unique_ptr<Type>>& params_type) -> std::unique_ptr<Type> {
    if (params_type.size() != 0) {
      throw WamonException("list.size error, params.size() == {}", params_type.size());
    }
    return TypeFactory<int>::Get();
  };

  handles[concat("list", "at")] = [](const std::unique_ptr<Type>& builtin_type,
                                     const std::vector<std::unique_ptr<Type>>& params_type) -> std::unique_ptr<Type> {
    if (params_type.size() != 1) {
      throw WamonException("list.at error, params.size() == {}", params_type.size());
    }
    if (!IsIntType(params_type[0])) {
      throw WamonException("list.at error, param's type == {}", params_type[0]->GetTypeInfo());
    }
    return GetElementType(builtin_type);
  };

  handles[concat("list", "insert")] =
      [](const std::unique_ptr<Type>& builtin_type,
         const std::vector<std::unique_ptr<Type>>& params_type) -> std::unique_ptr<Type> {
    if (params_type.size() != 2) {
      throw WamonException("list.insert error, params.size() == {}", params_type.size());
    }

    if (!IsIntType(params_type[0]) || !IsSameType(GetElementType(builtin_type), params_type[1])) {
      throw WamonException("list.insert error, param's type == {} {}", params_type[0]->GetTypeInfo(),
                           params_type[1]->GetTypeInfo());
    }
    return GetVoidType();
  };

  handles[concat("list", "push_back")] =
      [](const std::unique_ptr<Type>& builtin_type,
         const std::vector<std::unique_ptr<Type>>& params_type) -> std::unique_ptr<Type> {
    if (params_type.size() != 1) {
      throw WamonException("list.push_back error, params.size() == {}", params_type.size());
    }
    if (!IsSameType(GetElementType(builtin_type), params_type[0])) {
      throw WamonException("list.push_back error, params type dismatch : {} != {}",
                           GetElementType(builtin_type)->GetTypeInfo(), params_type[0]->GetTypeInfo());
    }
    return GetVoidType();
  };

  handles[concat("list", "pop_back")] =
      [](const std::unique_ptr<Type>& builtin_type,
         const std::vector<std::unique_ptr<Type>>& params_type) -> std::unique_ptr<Type> {
    if (params_type.size() != 0) {
      throw WamonException("list.pop_back error, params.size() == {}", params_type.size());
    }
    return GetVoidType();
  };

  handles[concat("list", "resize")] =
      [](const std::unique_ptr<Type>& builtin_type,
         const std::vector<std::unique_ptr<Type>>& params_type) -> std::unique_ptr<Type> {
    if (params_type.size() != 1) {
      throw WamonException("list.resize error, params.size() == {}", params_type.size());
    }
    if (!IsIntType(params_type[0])) {
      throw WamonException("list.resize error, params type == {}", params_type[0]->GetTypeInfo());
    }
    return GetVoidType();
  };

  handles[concat("list", "erase")] =
      [](const std::unique_ptr<Type>& builtin_type,
         const std::vector<std::unique_ptr<Type>>& params_type) -> std::unique_ptr<Type> {
    if (params_type.size() != 1) {
      throw WamonException("list.erase error, params.size() == {}", params_type.size());
    }
    if (!IsIntType(params_type[0])) {
      throw WamonException("list.erase error, params type == {}", params_type[0]->GetTypeInfo());
    }
    return GetVoidType();
  };

  handles[concat("list", "clear")] =
      [](const std::unique_ptr<Type>& builtin_type,
         const std::vector<std::unique_ptr<Type>>& params_type) -> std::unique_ptr<Type> {
    if (params_type.size() != 0) {
      throw WamonException("list.clear error, params.size() == {}", params_type.size());
    }
    return GetVoidType();
  };

  handles[concat("list", "empty")] =
      [](const std::unique_ptr<Type>& builtin_type,
         const std::vector<std::unique_ptr<Type>>& params_type) -> std::unique_ptr<Type> {
    if (params_type.size() != 0) {
      throw WamonException("list.empty error, params.size() == {}", params_type.size());
    }
    return TypeFactory<bool>::Get();
  };
}

static void register_builtin_type_method_handle(std::unordered_map<std::string, InnerTypeMethod::HandleType>& handles) {
  handles[concat("string", "len")] = [](std::shared_ptr<Variable>& obj, std::vector<std::shared_ptr<Variable>>&& params,
                                        Interpreter& ip) -> std::shared_ptr<Variable> {
    assert(params.empty());
    int ret = AsStringVariable(obj)->GetValue().size();
    return std::make_shared<IntVariable>(ret, Variable::ValueCategory::RValue);
  };

  handles[concat("string", "at")] = [](std::shared_ptr<Variable>& obj, std::vector<std::shared_ptr<Variable>>&& params,
                                       Interpreter& ip) -> std::shared_ptr<Variable> {
    assert(params.size() == 1);
    const std::string& v = AsStringVariable(obj)->GetValue();
    int index = AsIntVariable(params[0])->GetValue();
    if (static_cast<size_t>(index) >= v.size()) {
      throw WamonException("string.at error, index out of range : {} >= {}", index, v.size());
    }
    return std::make_shared<ByteVariable>(v[index], Variable::ValueCategory::RValue);
  };

  handles[concat("string", "append")] = [](std::shared_ptr<Variable>& obj,
                                           std::vector<std::shared_ptr<Variable>>&& params,
                                           Interpreter& ip) -> std::shared_ptr<Variable> {
    assert(params.size() == 1);
    std::string& v = AsStringVariable(obj)->GetValue();
    if (IsStringType(params[0]->GetType())) {
      const auto& p0 = AsStringVariable(params[0])->GetValue();
      v.append(p0);
    } else {
      char c = static_cast<char>(AsByteVariable(params[0])->GetValue());
      v.push_back(c);
    }
    return GetVoidVariable();
  };

  handles[concat("list", "size")] = [](std::shared_ptr<Variable>& obj, std::vector<std::shared_ptr<Variable>>&& params,
                                       Interpreter& ip) -> std::shared_ptr<Variable> {
    assert(params.empty());
    int ret = AsListVariable(obj)->Size();
    return std::make_shared<IntVariable>(ret, Variable::ValueCategory::RValue);
  };

  handles[concat("list", "at")] = [](std::shared_ptr<Variable>& obj, std::vector<std::shared_ptr<Variable>>&& params,
                                     Interpreter& ip) -> std::shared_ptr<Variable> {
    assert(params.size() == 1);
    int index = AsIntVariable(params[0])->GetValue();
    return AsListVariable(obj)->at(index);
  };

  handles[concat("list", "insert")] = [](std::shared_ptr<Variable>& obj,
                                         std::vector<std::shared_ptr<Variable>>&& params,
                                         Interpreter& ip) -> std::shared_ptr<Variable> {
    assert(params.size() == 2);
    int index = AsIntVariable(params[0])->GetValue();
    AsListVariable(obj)->Insert(index, params[1]);
    return GetVoidVariable();
  };

  handles[concat("list", "push_back")] = [](std::shared_ptr<Variable>& obj,
                                            std::vector<std::shared_ptr<Variable>>&& params,
                                            Interpreter& ip) -> std::shared_ptr<Variable> {
    assert(params.size() == 1);
    AsListVariable(obj)->PushBack(params[0]);
    return GetVoidVariable();
  };

  handles[concat("list", "pop_back")] = [](std::shared_ptr<Variable>& obj,
                                           std::vector<std::shared_ptr<Variable>>&& params,
                                           Interpreter& ip) -> std::shared_ptr<Variable> {
    assert(params.size() == 0);
    AsListVariable(obj)->PopBack();
    return GetVoidVariable();
  };

  handles[concat("list", "resize")] = [](std::shared_ptr<Variable>& obj,
                                         std::vector<std::shared_ptr<Variable>>&& params,
                                         Interpreter& ip) -> std::shared_ptr<Variable> {
    assert(params.size() == 1);
    AsListVariable(obj)->Resize(AsIntVariable(params[0])->GetValue(), ip);
    return GetVoidVariable();
  };

  handles[concat("list", "erase")] = [](std::shared_ptr<Variable>& obj, std::vector<std::shared_ptr<Variable>>&& params,
                                        Interpreter& ip) -> std::shared_ptr<Variable> {
    assert(params.size() == 1);
    AsListVariable(obj)->Erase(AsIntVariable(params[0])->GetValue());
    return GetVoidVariable();
  };

  handles[concat("list", "clear")] = [](std::shared_ptr<Variable>& obj, std::vector<std::shared_ptr<Variable>>&& params,
                                        Interpreter& ip) -> std::shared_ptr<Variable> {
    assert(params.size() == 0);
    AsListVariable(obj)->Clear();
    return GetVoidVariable();
  };

  handles[concat("list", "empty")] = [](std::shared_ptr<Variable>& obj, std::vector<std::shared_ptr<Variable>>&& params,
                                        Interpreter& ip) -> std::shared_ptr<Variable> {
    assert(params.size() == 0);
    bool empty = AsListVariable(obj)->Size() == 0;
    return std::make_shared<BoolVariable>(empty, Variable::ValueCategory::RValue);
  };
}

InnerTypeMethod::InnerTypeMethod() {
  register_builtin_type_method_check(checks_);
  // todo : 在handle函数中处理值型别
  register_builtin_type_method_handle(handles_);
}

}  // namespace wamon
