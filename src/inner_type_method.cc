#include "wamon/inner_type_method.h"

#include "wamon/exception.h"
#include "wamon/package_unit.h"

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
      throw WamonExecption("string.len error, params.size() == {}", params_type.size());
    }
    return TypeFactory<int>::Get();
  };

  handles[concat("string", "at")] = [](const std::unique_ptr<Type>& builtin_type,
                                       const std::vector<std::unique_ptr<Type>>& params_type) -> std::unique_ptr<Type> {
    if (params_type.size() != 1) {
      throw WamonExecption("string.at error, params.size() == {}", params_type.size());
    }
    if (!IsIntType(params_type[0])) {
      throw WamonExecption("string.at error, param's type == {}", params_type[0]->GetTypeInfo());
    }
    return TypeFactory<unsigned char>::Get();
  };

  handles[concat("list", "size")] = [](const std::unique_ptr<Type>& builtin_type,
                                       const std::vector<std::unique_ptr<Type>>& params_type) -> std::unique_ptr<Type> {
    if (params_type.size() != 0) {
      throw WamonExecption("list.size error, params.size() == {}", params_type.size());
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

  handles[concat("list", "insert")] =
      [](const std::unique_ptr<Type>& builtin_type,
         const std::vector<std::unique_ptr<Type>>& params_type) -> std::unique_ptr<Type> {
    if (params_type.size() != 2 || !IsIntType(params_type[0]) ||
        static_cast<ListType*>(builtin_type.get())->GetHoldType()->GetTypeInfo() != params_type[1]->GetTypeInfo()) {
      throw_params_type_check_error_exception("list", "at", "invalid params count or invalid params type");
    }
    return GetVoidType();
  };

  handles[concat("list", "push_back")] =
      [](const std::unique_ptr<Type>& builtin_type,
         const std::vector<std::unique_ptr<Type>>& params_type) -> std::unique_ptr<Type> {
    if (params_type.size() != 1) {
      throw WamonExecption("list.push_back error, params.size() == {}", params_type.size());
    }
    if (!IsSameType(GetElementType(builtin_type), params_type[0])) {
      throw WamonExecption("list.push_back error, params type dismatch : {} != {}",
                           GetElementType(builtin_type)->GetTypeInfo(), params_type[0]->GetTypeInfo());
    }
    return GetVoidType();
  };

  handles[concat("list", "pop_back")] =
      [](const std::unique_ptr<Type>& builtin_type,
         const std::vector<std::unique_ptr<Type>>& params_type) -> std::unique_ptr<Type> {
    if (params_type.size() != 0) {
      throw WamonExecption("list.pop_back error, params.size() == {}", params_type.size());
    }
    return GetVoidType();
  };

  handles[concat("list", "resize")] =
      [](const std::unique_ptr<Type>& builtin_type,
         const std::vector<std::unique_ptr<Type>>& params_type) -> std::unique_ptr<Type> {
    if (params_type.size() != 1) {
      throw WamonExecption("list.resize error, params.size() == {}", params_type.size());
    }
    if (!IsIntType(params_type[0])) {
      throw WamonExecption("list.resize error, params type == {}", params_type[0]->GetTypeInfo());
    }
    return GetVoidType();
  };
}

static void register_builtin_type_method_handle(std::unordered_map<std::string, InnerTypeMethod::HandleType>& handles) {
  handles[concat("string", "len")] = [](std::shared_ptr<Variable>& obj, std::vector<std::shared_ptr<Variable>>&& params,
                                        const PackageUnit& pu) -> std::shared_ptr<Variable> {
    assert(params.empty());
    int ret = AsStringVariable(obj)->GetValue().size();
    return std::make_shared<IntVariable>(ret, Variable::ValueCategory::RValue, "");
  };

  handles[concat("string", "at")] = [](std::shared_ptr<Variable>& obj, std::vector<std::shared_ptr<Variable>>&& params,
                                       const PackageUnit& pu) -> std::shared_ptr<Variable> {
    assert(params.size() == 1);
    const std::string& v = AsStringVariable(obj)->GetValue();
    int index = AsIntVariable(params[0])->GetValue();
    if (static_cast<size_t>(index) >= v.size()) {
      throw WamonExecption("string.at error, index out of range : {} >= {}", index, v.size());
    }
    return std::make_shared<ByteVariable>(v[index], Variable::ValueCategory::RValue, "");
  };

  handles[concat("list", "size")] = [](std::shared_ptr<Variable>& obj, std::vector<std::shared_ptr<Variable>>&& params,
                                       const PackageUnit& pu) -> std::shared_ptr<Variable> {
    assert(params.empty());
    int ret = AsListVariable(obj)->Size();
    return std::make_shared<IntVariable>(ret, Variable::ValueCategory::RValue, "");
  };

  handles[concat("list", "at")] = [](std::shared_ptr<Variable>& obj, std::vector<std::shared_ptr<Variable>>&& params,
                                     const PackageUnit& pu) -> std::shared_ptr<Variable> {
    assert(params.size() == 1);
    int index = AsIntVariable(params[0])->GetValue();
    return AsListVariable(obj)->at(index);
  };

  handles[concat("list", "insert")] = [](std::shared_ptr<Variable>& obj,
                                         std::vector<std::shared_ptr<Variable>>&& params,
                                         const PackageUnit& pu) -> std::shared_ptr<Variable> {
    assert(params.size() == 2);
    int index = AsIntVariable(params[0])->GetValue();
    AsListVariable(obj)->Insert(index, params[1]);
    return GetVoidVariable();
  };

  handles[concat("list", "push_back")] = [](std::shared_ptr<Variable>& obj,
                                            std::vector<std::shared_ptr<Variable>>&& params,
                                            const PackageUnit& pu) -> std::shared_ptr<Variable> {
    assert(params.size() == 1);
    AsListVariable(obj)->PushBack(params[0]);
    return GetVoidVariable();
  };

  handles[concat("list", "pop_back")] = [](std::shared_ptr<Variable>& obj,
                                           std::vector<std::shared_ptr<Variable>>&& params,
                                           const PackageUnit& pu) -> std::shared_ptr<Variable> {
    assert(params.size() == 0);
    AsListVariable(obj)->PopBack();
    return GetVoidVariable();
  };

  handles[concat("list", "resize")] = [](std::shared_ptr<Variable>& obj,
                                         std::vector<std::shared_ptr<Variable>>&& params,
                                         const PackageUnit& pu) -> std::shared_ptr<Variable> {
    assert(params.size() == 1);
    AsListVariable(obj)->Resize(AsIntVariable(params[0])->GetValue(), pu);
    return GetVoidVariable();
  };
}

InnerTypeMethod::InnerTypeMethod() {
  register_builtin_type_method_check(checks_);
  register_builtin_type_method_handle(handles_);
}

}  // namespace wamon
