#include "wamon/variable.h"

#include "wamon/interpreter.h"
#include "wamon/struct_def.h"

namespace wamon {

std::unique_ptr<Variable> VariableFactory(const std::unique_ptr<Type>& type, Variable::ValueCategory vc,
                                          const std::string& name, Interpreter& interpreter) {
  if (IsBuiltInType(type)) {
    std::string typeinfo = type->GetTypeInfo();
    if (typeinfo == "string") {
      return std::make_unique<StringVariable>("", vc, name);
    }
    if (typeinfo == "void") {
      return std::make_unique<VoidVariable>();
    }
    if (typeinfo == "bool") {
      return std::make_unique<BoolVariable>(true, vc, name);
    }
    if (typeinfo == "int") {
      return std::make_unique<IntVariable>(0, vc, name);
    }
    if (typeinfo == "double") {
      return std::make_unique<DoubleVariable>(0.0, vc, name);
    }
    if (typeinfo == "byte") {
      return std::make_unique<ByteVariable>(0, vc, name);
    }
  }
  if (type->IsBasicType() == true) {
    auto struct_def = interpreter.GetPackageUnit().FindStruct(type->GetTypeInfo());
    assert(struct_def != nullptr);
    return std::make_unique<StructVariable>(struct_def, vc, interpreter, name);
  }
  if (IsPtrType(type)) {
    return std::make_unique<PointerVariable>(GetHoldType(type), vc, name);
  }
  if (IsListType(type)) {
    return std::make_unique<ListVariable>(GetElementType(type), vc, name);
  }
  if (IsFuncType(type)) {
    return std::make_unique<FunctionVariable>(GetParamType(type), GetReturnType(type), vc, name);
  }
  throw WamonExecption("VariableFactory error, not implement now.");
}

StructVariable::StructVariable(const StructDef* sd, ValueCategory vc, Interpreter& i, const std::string& name)
    : Variable(std::make_unique<BasicType>(sd->GetStructName()), vc, name), def_(sd), interpreter_(i) {}

std::shared_ptr<Variable> StructVariable::GetDataMemberByName(const std::string& name) {
  if (def_->IsTrait()) {
    check_trait_not_null(__FILE__, __LINE__);
    return AsStructVariable(trait_proxy_)->GetDataMemberByName(name);
  }
  for (auto& each : data_members_) {
    if (each.name == name) {
      return each.data;
    }
  }
  return nullptr;
}

void StructVariable::UpdateDataMemberByName(const std::string& name, std::shared_ptr<Variable> data) {
  if (def_->IsTrait()) {
    check_trait_not_null(__FILE__, __LINE__);
    AsStructVariable(trait_proxy_)->UpdateDataMemberByName(name, data);
    return;
  }
  data->ChangeTo(vc_);
  auto it =
      std::find_if(data_members_.begin(), data_members_.end(), [&](auto& each) -> bool { return each.name == name; });
  assert(it != data_members_.end());
  it->data = data;
}

void StructVariable::ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) {
  if (def_->IsTrait()) {
    if (fields[0] == nullptr) {
      trait_proxy_ = nullptr;
    } else if (fields[0]->IsRValue()) {
      trait_proxy_ = fields[0];
    } else {
      trait_proxy_ = fields[0]->Clone();
    }
    if (trait_proxy_ != nullptr) {
      trait_proxy_->ChangeTo(vc_);
    }
    return;
  }
  auto& members = def_->GetDataMembers();
  if (fields.size() != members.size()) {
    throw WamonExecption("StructVariable's ConstructByFields method error : fields.size() == {}", fields.size());
  }
  for (size_t i = 0; i < members.size(); ++i) {
    if (fields[i]->GetTypeInfo() != members[i].second->GetTypeInfo()) {
      throw WamonExecption("StructVariable's ConstructByFields method error : {}th type dismatch : {} != {}", i,
                           fields[i]->GetTypeInfo(), members[i].second->GetTypeInfo());
    }
    if (fields[i]->IsRValue()) {
      data_members_.push_back({members[i].first, std::move(fields[i])});
    } else {
      data_members_.push_back({members[i].first, fields[i]->Clone()});
    }
    data_members_.back().data->ChangeTo(vc_);
  }
}

void StructVariable::DefaultConstruct() {
  if (def_->IsTrait()) {
    trait_proxy_ = nullptr;
    return;
  }
  auto& members = def_->GetDataMembers();
  for (auto& each : members) {
    data_members_.push_back({each.first, VariableFactory(each.second, vc_, each.first, interpreter_)});
  }
}

std::unique_ptr<Variable> StructVariable::Clone() {
  if (def_->IsTrait()) {
    std::shared_ptr<Variable> proxy{nullptr};
    if (trait_proxy_ == nullptr) {
      ;  // do nothing
    } else if (IsRValue()) {
      proxy = trait_proxy_;
    } else {
      proxy = trait_proxy_->Clone();
    }
    auto ret = std::make_unique<StructVariable>(def_, ValueCategory::RValue, interpreter_, GetName());
    ret->ConstructByFields({proxy});
    return ret;
  }
  std::vector<std::shared_ptr<Variable>> variables;
  for (auto& each : data_members_) {
    if (each.data->IsRValue()) {
      variables.push_back(std::move(each.data));
    } else {
      variables.push_back(each.data->Clone());
      variables.back()->ChangeTo(ValueCategory::RValue);
    }
  }
  // all variable in variable is rvalue now
  auto ret = std::make_unique<StructVariable>(def_, ValueCategory::RValue, interpreter_, GetName());
  ret->ConstructByFields(variables);
  return ret;
}

bool StructVariable::trait_compare(StructVariable* lv, StructVariable* rv) {
  if (lv->trait_proxy_ == nullptr && rv->trait_proxy_ == nullptr) {
    return true;
  }
  if (lv->trait_proxy_ == nullptr || rv->trait_proxy_ == nullptr) {
    return false;
  }
  // 类型分析阶段保证
  assert(lv->def_ == rv->def_);
  for (auto& each : lv->def_->GetDataMembers()) {
    auto tmp = lv->GetDataMemberByName(each.first);
    auto tmp2 = rv->GetDataMemberByName(each.first);
    if (tmp->Compare(tmp2) == false) {
      return false;
    }
  }
  return true;
}

void StructVariable::trait_assign(StructVariable* lv, StructVariable* rv) {
  // 类型分析阶段保证
  assert(lv->def_ == rv->def_);
  if (lv->trait_proxy_ == nullptr && rv->trait_proxy_ == nullptr) {
    return;
  }
  if (rv->trait_proxy_ == nullptr) {
    lv->trait_proxy_ = nullptr;
    return;
  }
  if (lv->trait_proxy_ == nullptr) {
    lv->trait_proxy_ = rv->trait_proxy_->Clone();
    lv->trait_proxy_->ChangeTo(lv->vc_);
  }
  for (auto& each : lv->def_->GetDataMembers()) {
    auto tmp = rv->GetDataMemberByName(each.first);
    if (rv->IsRValue()) {
      lv->UpdateDataMemberByName(each.first, tmp);
    } else {
      lv->UpdateDataMemberByName(each.first, tmp->Clone());
    }
  }
}

// 目前仅支持相同类型的trait间的比较和赋值
bool StructVariable::Compare(const std::shared_ptr<Variable>& other) {
  check_compare_type_match(other);
  if (def_->IsTrait()) {
    return trait_compare(this, AsStructVariable(other));
  }
  // trait compare todo
  StructVariable* other_struct = static_cast<StructVariable*>(other.get());
  for (size_t index = 0; index < data_members_.size(); ++index) {
    if (data_members_[index].data->Compare(other_struct->data_members_[index].data) == false) {
      return false;
    }
  }
  return true;
}

void StructVariable::Assign(const std::shared_ptr<Variable>& other) {
  check_compare_type_match(other);
  if (def_->IsTrait()) {
    return trait_assign(this, AsStructVariable(other));
  }
  StructVariable* other_struct = static_cast<StructVariable*>(other.get());
  if (other_struct->IsRValue()) {
    for (size_t index = 0; index < data_members_.size(); ++index) {
      data_members_[index].data = std::move(other_struct->data_members_[index].data);
      data_members_[index].data->ChangeTo(vc_);
    }
  } else {
    for (size_t index = 0; index < data_members_.size(); ++index) {
      data_members_[index].data->Assign(other_struct->data_members_[index].data);
      data_members_[index].data->ChangeTo(vc_);
    }
  }
}

void StructVariable::ChangeTo(ValueCategory vc) {
  vc_ = vc;
  if (def_->IsTrait()) {
    trait_proxy_->ChangeTo(vc);
    return;
  }
  for (auto& each : data_members_) {
    each.data->ChangeTo(vc);
  }
}

void StructVariable::Print(Output& output) {
  assert(def_ != nullptr);
  if (def_->IsTrait()) {
    output.OutputFormat("struct trait {}", def_->GetStructName());
    return;
  }
  output.OutputFormat("struct {} : [ ", def_->GetStructName());
  for (size_t index = 0; index < data_members_.size(); ++index) {
    data_members_[index].data->Print(output);
    output.OutPutString(" ");
  }
  output.OutPutString(" ]");
}

void PointerVariable::ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) {
  if (fields.size() != 1) {
    throw WamonExecption("PointerVariable's ConstructByFields method error : fields.size() == {}", fields.size());
  }
  if (GetTypeInfo() != fields[0]->GetTypeInfo()) {
    throw WamonExecption("PointerVariable's ConstructByFields method error, type dismatch : {} != {}",
                         fields[0]->GetTypeInfo(), GetTypeInfo());
  }
  obj_ = AsPointerVariable(fields[0])->GetHoldVariable();
}

void PointerVariable::DefaultConstruct() { obj_.reset(); }

std::unique_ptr<Variable> PointerVariable::Clone() {
  auto ret = std::make_unique<PointerVariable>(obj_.lock()->GetType(), ValueCategory::RValue, "");
  ret->SetHoldVariable(obj_.lock());
  return ret;
}

void ListVariable::PushBack(std::shared_ptr<Variable> element) {
  if (element->IsRValue()) {
    elements_.push_back(std::move(element));
  } else {
    elements_.push_back(element->Clone());
  }
  elements_.back()->ChangeTo(vc_);
}

void ListVariable::PopBack() {
  if (elements_.empty()) {
    throw WamonExecption("List pop back error, empty list");
  }
  elements_.pop_back();
}

void ListVariable::ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) {
  for (auto& each : fields) {
    if (each->GetTypeInfo() != element_type_->GetTypeInfo()) {
      throw WamonExecption("ListVariable::ConstructByFields error, type dismatch : {} != {}", each->GetTypeInfo(),
                           element_type_->GetTypeInfo());
    }
    if (each->IsRValue()) {
      elements_.push_back(std::move(each));
    } else {
      elements_.push_back(each->Clone());
    }
    elements_.back()->ChangeTo(vc_);
  }
}

void ListVariable::DefaultConstruct() {}

std::unique_ptr<Variable> ListVariable::Clone() {
  std::vector<std::shared_ptr<Variable>> elements;
  for (auto& each : elements_) {
    if (each->IsRValue()) {
      elements.push_back(std::move(each));
    } else {
      elements.push_back(each->Clone());
      elements.back()->ChangeTo(ValueCategory::RValue);
    }
  }
  auto ret = std::make_unique<ListVariable>(element_type_->Clone(), ValueCategory::RValue, "");
  ret->elements_ = std::move(elements);
  return ret;
}

void FunctionVariable::ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) {
  if (fields.size() != 1) {
    throw WamonExecption("FunctionVariable's ConstructByFields method error : fields.size() == {}", fields.size());
  }
  if (IsBasicType(fields[0]->GetType()) && !IsBuiltInType(fields[0]->GetType())) {
    // structtype
    if (fields[0]->IsRValue()) {
      obj_ = std::move(fields[0]);
    } else {
      obj_ = fields[0]->Clone();
    }
    obj_->ChangeTo(vc_);
    return;
  }
  if (fields[0]->GetTypeInfo() != GetTypeInfo()) {
    throw WamonExecption("FunctionVariable's ConstructByFields method error, type dismatch : {} != {}",
                         fields[0]->GetTypeInfo(), GetTypeInfo());
  }
  auto other = AsFunctionVariable(fields[0]);
  if (fields[0]->IsRValue()) {
    capture_variables_ = std::move(other->capture_variables_);
  } else {
    for (auto& each : other->capture_variables_) {
      capture_variables_.push_back(each->Clone());
    }
  }
  func_name_ = AsStringVariable(fields[0])->GetValue();
}

void FunctionVariable::DefaultConstruct() {}

std::unique_ptr<Variable> FunctionVariable::Clone() {
  std::vector<std::unique_ptr<Type>> param_type;
  auto type = GetType();
  auto fun_type = dynamic_cast<FuncType*>(type.get());
  for (auto&& each : fun_type->GetParamType()) {
    param_type.push_back(each->Clone());
  }
  auto obj = std::make_unique<FunctionVariable>(std::move(param_type), fun_type->GetReturnType()->Clone(),
                                                ValueCategory::RValue, "");
  obj->SetFuncName(func_name_);
  if (obj_) {
    if (IsRValue()) {
      obj->SetObj(std::move(obj_));
    } else {
      obj->SetObj(obj_->Clone());
    }
    obj->ChangeTo(ValueCategory::RValue);
  }
  if (IsRValue()) {
    obj->capture_variables_ = std::move(capture_variables_);
  } else {
    for (auto& each : capture_variables_) {
      obj->capture_variables_.push_back(each->Clone());
    }
  }
  for (auto& each : obj->capture_variables_) {
    each->ChangeTo(ValueCategory::RValue);
  }
  return obj;
}

}  // namespace wamon
