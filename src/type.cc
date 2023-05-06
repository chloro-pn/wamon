#include "wamon/type.h"
#include "wamon/ast.h"
#include "wamon/package_unit.h"

namespace wamon {

std::unique_ptr<Type> BasicType::Clone() const {
  return std::make_unique<BasicType>(type_name_);
}

int64_t ArrayType::GetCount() const {
  return count_expr_->num_;
}

std::string ArrayType::GetTypeInfo() const { 
  return "array[" + std::to_string(count_expr_->num_) + "](" + hold_type_->GetTypeInfo() + ")";
}

std::unique_ptr<Type> PointerType::Clone() const {
  auto tmp = hold_type_->Clone();
  auto ret = std::make_unique<PointerType>();
  ret->SetHoldType(std::move(tmp));
  return tmp;
}

std::unique_ptr<Type> ArrayType::Clone() const {
  auto tmp = std::make_unique<IntIteralExpr>();
  tmp->SetIntIter(count_expr_->num_);
  auto tmp2 = hold_type_->Clone();
  auto tmp3 = std::make_unique<ArrayType>();
  tmp3->SetCount(std::move(tmp));
  tmp3->SetHoldType(std::move(tmp2));
  return tmp3;
}

std::unique_ptr<Type> FuncType::Clone() const {
  auto tmp = std::make_unique<FuncType>();
  std::vector<std::unique_ptr<Type>> params;
  std::unique_ptr<Type> ret = return_type_->Clone();
  for(auto& each : param_type_) {
    params.emplace_back(each->Clone());
  }
  tmp->SetParamTypeAndReturnType(std::move(params), std::move(ret));
  return tmp;
}

void ArrayType::SetCount(std::unique_ptr<IntIteralExpr>&& count) {
  count_expr_ = std::move(count);
}

// todo : 支持重载了operator()的类型初始化callable_object
void CheckCanConstructBy(const PackageUnit& pu, const std::unique_ptr<Type>& var_type, const std::vector<std::unique_ptr<Type>>& param_types) {
  if (IsVoidType(var_type)) {
    throw WamonExecption("var's type should not be void");
  }
  // built-in类型、原生函数到callable_object类型
  if (param_types.size() == 1 && IsSameType(var_type, param_types[0])) {
    return;
  }
  if (IsArrayType(var_type)) {
    ArrayType* type = static_cast<ArrayType*>(var_type.get());
    if (static_cast<size_t>(type->GetCount()) != param_types.size()) {
      throw WamonExecption("array type's constructors should have the same number as the definition, but {} != {}", type->GetCount(), param_types.size());
    }
    for(size_t i = 0; i < param_types.size(); ++i) {
      if (!IsSameType(type->GetHoldType(), param_types[i])) {
        throw WamonExecption("array type's {}th constructor type invalid, {} != {}", i, type->GetHoldType()->GetTypeInfo(), param_types[i]->GetTypeInfo());
      }
    }
    return;
  }
  if (IsBasicType(var_type)) {
    auto struct_def = pu.FindStruct(var_type->GetTypeInfo());
    if (struct_def == nullptr) {
      throw WamonExecption("construct check error, invalid struct name {}", var_type->GetTypeInfo());
    }
    const auto& dms = struct_def->GetDataMembers();
    if (dms.size() != param_types.size()) {
      throw WamonExecption("struct construct check error, {} != {}", dms.size(), param_types.size());
    }
    for(size_t i = 0; i < dms.size(); ++i) {
      if (!IsSameType(dms[i].second, param_types[i])) {
        throw WamonExecption("struct construct check error, {}th data member is constructd by the dismatch type, {} != {}", i, dms[i].second->GetTypeInfo(), param_types[i]->GetTypeInfo());
      }
    }
    return;
  }
  std::vector<std::string> type_infos;
  for(auto& each : param_types) {
    type_infos.push_back(each->GetTypeInfo());
  }
  throw WamonExecption("construct check error,type {} can not be constructed by {} ", var_type->GetTypeInfo(), fmt::join(type_infos, ", "));
}

}