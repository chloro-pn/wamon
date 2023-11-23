#include "wamon/type.h"

#include "wamon/ast.h"
#include "wamon/operator_def.h"
#include "wamon/package_unit.h"

namespace wamon {

std::unique_ptr<Type> BasicType::Clone() const { return std::make_unique<BasicType>(type_name_); }

std::string ListType::GetTypeInfo() const { return "list(" + hold_type_->GetTypeInfo() + ")"; }

std::unique_ptr<Type> PointerType::Clone() const {
  auto tmp = hold_type_->Clone();
  auto ret = std::make_unique<PointerType>(std::move(tmp));
  return ret;
}

std::unique_ptr<Type> ListType::Clone() const {
  auto tmp = hold_type_->Clone();
  auto tmp2 = std::make_unique<ListType>(std::move(tmp));
  return tmp2;
}

std::unique_ptr<Type> FuncType::Clone() const {
  std::vector<std::unique_ptr<Type>> params;
  std::unique_ptr<Type> ret = return_type_->Clone();
  for (auto& each : param_type_) {
    params.emplace_back(each->Clone());
  }
  return std::make_unique<FuncType>(std::move(params), std::move(ret));
}

// todo : 支持重载了operator()的类型初始化callable_object
void CheckCanConstructBy(const PackageUnit& pu, const std::unique_ptr<Type>& var_type,
                         const std::vector<std::unique_ptr<Type>>& param_types) {
  if (IsVoidType(var_type)) {
    throw WamonExecption("CheckCanConstructBy check error, var's type should not be void");
  }
  // 原生函数到callable_object类型
  if (IsFuncType(var_type)) {
    if (param_types.size() == 1 && IsSameType(var_type, param_types[0])) {
      return;
    }
    if (param_types.size() == 1 && IsBasicType(param_types[0]) && !IsBuiltInType(param_types[0])) {
      // 重载了()运算符的结构体类型的对象
      auto method_name = OperatorDef::CreateName(Token::LEFT_PARENTHESIS, GetParamType(var_type));
      auto struct_def = pu.FindStruct(param_types[0]->GetTypeInfo());
      if (struct_def != nullptr) {
        auto method_def = struct_def->GetMethod(method_name);
        if (method_def != nullptr) {
          return;
        }
      }
    }
  }
  // built-in类型
  if (IsBuiltInType(var_type)) {
    if (param_types.size() != 1 || !IsSameType(var_type, param_types[0])) {
      throw WamonExecption(
          "CheckCanConstructBy check error, builtin type {} should have only one param that has the same type",
          var_type->GetTypeInfo());
    }
    return;
  }
  // ptr
  if (param_types.size() == 1 && IsSameType(var_type, param_types[0])) {
    return;
  }
  if (IsListType(var_type)) {
    ListType* type = static_cast<ListType*>(var_type.get());
    for (size_t i = 0; i < param_types.size(); ++i) {
      if (!IsSameType(type->GetHoldType(), param_types[i])) {
        throw WamonExecption("list type's {}th constructor type invalid, {} != {}", i,
                             type->GetHoldType()->GetTypeInfo(), param_types[i]->GetTypeInfo());
      }
    }
    return;
  }
  // struct 类型
  if (IsBasicType(var_type)) {
    auto struct_def = pu.FindStruct(var_type->GetTypeInfo());
    if (struct_def == nullptr) {
      throw WamonExecption("construct check error, invalid struct name {}", var_type->GetTypeInfo());
    }
    const auto& dms = struct_def->GetDataMembers();
    if (dms.size() != param_types.size()) {
      throw WamonExecption("struct construct check error, {} != {}", dms.size(), param_types.size());
    }
    for (size_t i = 0; i < dms.size(); ++i) {
      if (!IsSameType(dms[i].second, param_types[i])) {
        throw WamonExecption(
            "struct construct check error, {}th data member is constructd by the dismatch type, {} != {}", i,
            dms[i].second->GetTypeInfo(), param_types[i]->GetTypeInfo());
      }
    }
    return;
  }
  std::vector<std::string> type_infos;
  for (auto& each : param_types) {
    type_infos.push_back(each->GetTypeInfo());
  }
  throw WamonExecption("construct check error,type {} can not be constructed by {} ", var_type->GetTypeInfo(),
                       fmt::join(type_infos, ", "));
}

}  // namespace wamon
