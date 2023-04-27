#include "wamon/type.h"
#include "wamon/ast.h"

namespace wamon {

std::unique_ptr<Type> BasicType::Clone() const {
  return std::make_unique<BasicType>(type_name_);
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

}