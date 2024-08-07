#include "wamon/type.h"

#include "wamon/ast.h"
#include "wamon/operator_def.h"
#include "wamon/package_unit.h"

namespace wamon {

std::unique_ptr<Type> BasicType::Clone() const {
  auto tmp = std::make_unique<BasicType>(type_name_);
  return tmp;
}

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

bool CheckTraitConstraint(const PackageUnit& pu, const std::unique_ptr<Type>& trait_type,
                          const std::unique_ptr<Type>& param_type, std::string& reason) {
  const std::string trait_name = trait_type->GetTypeInfo();
  const std::string param_name = param_type->GetTypeInfo();
  auto trait_def = pu.FindStruct(trait_name);
  auto param_def = pu.FindStruct(param_name);
  if (trait_def == nullptr || param_def == nullptr) {
    throw WamonException("CheckTraitConstraint error, trait_type or param_type not exist : {} {}", trait_name,
                         param_name);
  }
  if (trait_def->IsTrait() == false) {
    throw WamonException("CheckTraitConstraint error, {} should be struct trait", trait_name);
  }
  /* param_name 可以是struct trait
  if (param_def->IsTrait() == true) {
    throw WamonException("CheckTraitConstraint error, {} should not be struct trait", param_name);
  }
  */
  // 在param_type中查找trait_type中约定的数据成员和方法，根据名字查找，匹配类型
  for (auto& each : trait_def->GetDataMembers()) {
    auto data_type = param_def->GetDataMemberType<false>(each.first);
    if (data_type == nullptr) {
      reason = fmt::format("CheckTraitConstraint error, data_type {} not found in {}", each.first, param_name);
      return false;
    }
    if (!IsSameType(data_type, each.second)) {
      reason = fmt::format("CheckTraitConstraint error, data member {}'s type mismatch : {} != {}", each.first,
                           each.second->GetTypeInfo(), data_type->GetTypeInfo());
      return false;
    }
  }
  for (auto& each : trait_def->GetMethods()) {
    auto method_def = param_def->GetMethod(each->GetMethodName());
    if (method_def == nullptr) {
      reason = fmt::format("CheckTraitConstraint error, method {} not found in {}", each->GetMethodName(), param_name);
      return false;
    }
    if (!IsSameType(method_def->GetType(), each->GetType())) {
      reason = fmt::format("CheckTraitConstraint error, method {} type mismatch : {} != {}", each->GetMethodName(),
                           method_def->GetType()->GetTypeInfo(), each->GetType()->GetTypeInfo());
      return false;
    }
  }
  return true;
}

namespace detail {

void CheckCanConstructBy(const PackageUnit& pu, const std::unique_ptr<Type>& var_type,
                         const std::vector<std::unique_ptr<Type>>& param_types, bool is_ref) {
  if (IsVoidType(var_type)) {
    throw WamonException("CheckCanConstructBy check error, var's type should not be void");
  }
  std::vector<std::string> type_infos;
  for (auto& each : param_types) {
    type_infos.push_back(each->GetTypeInfo());
  }
  // 原生函数到callable_object类型
  if (IsFuncType(var_type)) {
    if (param_types.size() == 1 && IsSameType(var_type, param_types[0])) {
      return;
    }
    if (param_types.size() == 1 && IsStructOrEnumType(param_types[0])) {
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
    if (param_types.size() != 1) {
      throw WamonException(
          "CheckCanConstructBy check error, builtin type {} should have only one param that has the same type",
          var_type->GetTypeInfo());
    }
    if (!IsSameType(var_type, param_types[0])) {
      throw WamonException(
          "CheckCanConstructBy check error, builtin type {} should have only one param that has the same type",
          var_type->GetTypeInfo());
    }
    return;
  }
  // 所有相同类型的单个对象均可以执行构造（复制操作）
  // also handle enum here
  if (param_types.size() == 1 && IsSameType(var_type, param_types[0])) {
    return;
  }

  if (is_ref == true) {
    // ref构造如果执行到这里，则表示不是相同类型的单个对象，因此直接失败
    throw WamonException("ref construct check error, type ref {} can not be constructed by {}", var_type->GetTypeInfo(),
                         fmt::join(type_infos, ", "));
  }
  if (IsListType(var_type)) {
    ListType* type = static_cast<ListType*>(var_type.get());
    for (size_t i = 0; i < param_types.size(); ++i) {
      if (!IsSameType(type->GetHoldType(), param_types[i])) {
        throw WamonException("list type's {}th constructor type invalid, {} != {}", i,
                             type->GetHoldType()->GetTypeInfo(), param_types[i]->GetTypeInfo());
      }
    }
    return;
  }
  // struct 类型
  if (IsStructOrEnumType(var_type)) {
    // 这里不需要考虑Enum的情况，因为Enum的合理构造在 相同类型的单个对象构造中完成。
    auto struct_def = pu.FindStruct(var_type->GetTypeInfo());
    if (struct_def == nullptr) {
      throw WamonException("construct check error, invalid struct name {}", var_type->GetTypeInfo());
    }
    if (struct_def != nullptr) {
      // struct trait
      if (struct_def->IsTrait()) {
        if (param_types.size() != 1) {
          throw WamonException("struct trait {} construct check error, params should be only 1 but {}",
                               var_type->GetTypeInfo(), param_types.size());
        }
        std::string reason;
        if (CheckTraitConstraint(pu, var_type, param_types[0], reason) == false) {
          throw WamonException("struct trait {} construct check error, trait constraint check failed, reason : {}",
                               var_type->GetTypeInfo(), reason);
        }
        return;
      }
      const auto& dms = struct_def->GetDataMembers();
      if (dms.size() != param_types.size()) {
        throw WamonException("struct construct check error, {} != {}", dms.size(), param_types.size());
      }
      for (size_t i = 0; i < dms.size(); ++i) {
        if (!IsSameType(dms[i].second, param_types[i])) {
          throw WamonException(
              "struct construct check error, {}th data member is constructd by the dismatch type, {} != {}", i,
              dms[i].second->GetTypeInfo(), param_types[i]->GetTypeInfo());
        }
      }
      return;
    }
  }
  throw WamonException("CheckCanConstructBy check error, type {} can not be constructed by {} ",
                       var_type->GetTypeInfo(), fmt::join(type_infos, ", "));
}

}  // namespace detail

bool IsEnumType(const std::unique_ptr<Type>& type, const PackageUnit& pu) {
  if (IsStructOrEnumType(type) == false) {
    return false;
  }
  return pu.FindEnum(type->GetTypeInfo()) != nullptr;
}

}  // namespace wamon
