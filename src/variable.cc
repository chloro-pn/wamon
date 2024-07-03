#include "wamon/variable.h"

#include <cassert>

#include "wamon/enum_def.h"
#include "wamon/interpreter.h"
#include "wamon/struct_def.h"
#include "wamon/variable_bool.h"
#include "wamon/variable_byte.h"
#include "wamon/variable_double.h"
#include "wamon/variable_int.h"
#include "wamon/variable_string.h"
#include "wamon/variable_void.h"

namespace wamon {

std::shared_ptr<Variable> VariableFactory(const std::unique_ptr<Type>& type, Variable::ValueCategory vc,
                                          const std::string& name, Interpreter* ip) {
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
    if (ip == nullptr) {
      throw WamonException("VariableFactory error, ip == nullptr");
    }
    auto struct_def = ip->GetPackageUnit().FindStruct(type->GetTypeInfo());
    if (struct_def != nullptr) {
      return std::make_shared<StructVariable>(struct_def, vc, *ip, name);
    }
    // enum
    auto enum_def = ip->GetPackageUnit().FindEnum(type->GetTypeInfo());
    if (enum_def == nullptr) {
      throw WamonException("VariableFactory error, invalid struct and enum type {}", type->GetTypeInfo());
    }
    return std::make_shared<EnumVariable>(enum_def, vc, name);
  }
  if (IsPtrType(type)) {
    return std::make_unique<PointerVariable>(GetHoldType(type), vc, name);
  }
  if (IsListType(type)) {
    return std::make_unique<ListVariable>(GetElementType(type), vc, *ip, name);
  }
  if (IsFuncType(type)) {
    return std::make_unique<FunctionVariable>(GetParamType(type), GetReturnType(type), vc, name);
  }
  throw WamonException("VariableFactory error, invalid type {}", type->GetTypeInfo());
}

std::shared_ptr<Variable> VariableFactory(const std::unique_ptr<Type>& type, Variable::ValueCategory vc,
                                          const std::string& name, Interpreter& ip) {
  return VariableFactory(type, vc, name, &ip);
}

StructVariable::StructVariable(const StructDef* sd, ValueCategory vc, Interpreter& ip, const std::string& name)
    : Variable(std::make_unique<BasicType>(sd->GetStructName()), vc, name), def_(sd), trait_def_(def_), ip_(ip) {}

std::string StructVariable::GetTypeInfo() const { return trait_def_->GetStructName(); }

std::unique_ptr<Type> StructVariable::GetType() const {
  return std::make_unique<BasicType>(trait_def_->GetStructName());
}

std::shared_ptr<Variable> StructVariable::GetDataMemberByName(const std::string& name) {
  for (auto& each : data_members_) {
    if (each.name == name) {
      return each.data;
    }
  }
  return nullptr;
}

void StructVariable::AddDataMemberByName(const std::string& name, std::shared_ptr<Variable> data) {
  data_members_.push_back({name, data});
}

void StructVariable::UpdateDataMemberByName(const std::string& name, std::shared_ptr<Variable> data) {
  data->ChangeTo(vc_);
  auto it =
      std::find_if(data_members_.begin(), data_members_.end(), [&](auto& each) -> bool { return each.name == name; });
  if (it == data_members_.end()) {
    throw WamonException("StructVariable.UpdateDataMemberByName error, data member {} not exist", name);
  }
  it->data = data;
}

void StructVariable::ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) {
  assert(def_ == trait_def_);
  if (def_->IsTrait()) {
    assert(fields.size() == 1);
    StructVariable* other_struct = static_cast<StructVariable*>(fields[0].get());
    trait_construct(this, other_struct);
    def_ = other_struct->def_;
    assert(def_->IsTrait() == false);
    return;
  }
  auto& members = def_->GetDataMembers();
  if (fields.size() != members.size()) {
    throw WamonException("StructVariable's ConstructByFields method error : fields.size() == {}, type == {}",
                         fields.size(), def_->GetStructName());
  }
  for (size_t i = 0; i < members.size(); ++i) {
    if (!IsSameType(fields[i]->GetType(), members[i].second)) {
      throw WamonException("StructVariable's ConstructByFields method error : {}th type dismatch : {} != {}", i,
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
  trait_def_ = def_;
  data_members_.clear();
  auto& members = def_->GetDataMembers();
  for (auto& each : members) {
    auto member = VariableFactory(each.second, vc_, each.first, ip_);
    member->DefaultConstruct();
    data_members_.push_back({each.first, std::move(member)});
  }
}

std::shared_ptr<Variable> StructVariable::Clone() {
  std::vector<StructVariable::member> variables;
  for (auto& each : data_members_) {
    if (each.data->IsRValue()) {
      variables.push_back({each.name, std::move(each.data)});
    } else {
      variables.push_back({each.name, each.data->Clone()});
    }
  }
  // all variable in variables is rvalue now
  auto ret = std::make_shared<StructVariable>(def_, ValueCategory::RValue, ip_, "");
  ret->data_members_ = std::move(variables);
  ret->set_trait_def(trait_def_);
  return ret;
}

// fix ：析构函数中无法shared_from_this()，需要直接获取其指针
StructVariable::~StructVariable() {
  auto md = def_->GetDestructor();
  if (md != nullptr) {
    // 使用do nothing deleter确保不会delete this指针
    std::shared_ptr<StructVariable> myself(this, [](void*) {});
    ip_.CallMethod(myself, md, {});
  }
}

void StructVariable::trait_construct(StructVariable* lv, StructVariable* rv) {
  for (const auto& each : lv->trait_def_->GetDataMembers()) {
    auto tmp = rv->GetDataMemberByName(each.first);
    if (rv->IsRValue()) {
      assert(tmp->IsRValue());
      tmp->ChangeTo(lv->vc_);
      lv->AddDataMemberByName(each.first, tmp);
    } else {
      assert(tmp->IsRValue() == false);
      auto ttmp = tmp->Clone();
      ttmp->ChangeTo(lv->vc_);
      lv->AddDataMemberByName(each.first, ttmp);
    }
  }
}

bool StructVariable::trait_compare(StructVariable* lv, StructVariable* rv) {
  // 类型分析阶段保证
  assert(lv->trait_def_ == rv->trait_def_);
  for (const auto& each : lv->trait_def_->GetDataMembers()) {
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
  assert(lv->trait_def_ == rv->trait_def_);
  for (auto& each : lv->trait_def_->GetDataMembers()) {
    auto tmp = rv->GetDataMemberByName(each.first);
    if (rv->IsRValue()) {
      assert(tmp->IsRValue());
      tmp->ChangeTo(lv->vc_);
      lv->UpdateDataMemberByName(each.first, tmp);
    } else {
      assert(!tmp->IsRValue());
      auto ttmp = tmp->Clone();
      ttmp->ChangeTo(lv->vc_);
      lv->UpdateDataMemberByName(each.first, ttmp);
    }
  }
}

// 目前仅支持相同类型的trait间的比较和赋值
bool StructVariable::Compare(const std::shared_ptr<Variable>& other) {
  StructVariable* other_struct = static_cast<StructVariable*>(other.get());
  assert(trait_def_ == other_struct->trait_def_);
  if (trait_def_->IsTrait()) {
    return trait_compare(this, other_struct);
  }
  for (size_t index = 0; index < data_members_.size(); ++index) {
    if (data_members_[index].data->Compare(other_struct->data_members_[index].data) == false) {
      return false;
    }
  }
  return true;
}

void StructVariable::Assign(const std::shared_ptr<Variable>& other) {
  StructVariable* other_struct = static_cast<StructVariable*>(other.get());
  assert(trait_def_ == other_struct->trait_def_);
  if (trait_def_->IsTrait()) {
    return trait_assign(this, other_struct);
  }
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
  for (auto& each : data_members_) {
    assert(each.data != nullptr);
    each.data->ChangeTo(vc);
  }
}

nlohmann::json StructVariable::Print() {
  assert(def_ != nullptr);
  nlohmann::json obj;
  if (def_->IsTrait()) {
    obj["struct trait"] = def_->GetStructName();
    return obj;
  }
  obj["struct"] = def_->GetStructName();
  for (size_t index = 0; index < data_members_.size(); ++index) {
    obj["members"][data_members_[index].name] = data_members_[index].data->Print();
  }
  return obj;
}

EnumVariable::EnumVariable(const EnumDef* def, ValueCategory vc, const std::string& name)
    : Variable(std::make_unique<BasicType>(def->GetEnumName()), vc, name), def_(def) {}

void EnumVariable::ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) {
  if (fields.size() != 1) {
    throw WamonException("EnumVariable ConstructByFields error, param.size() == {}", fields.size());
  }
  if (GetTypeInfo() != fields[0]->GetTypeInfo()) {
    throw WamonException("EnumVariable ConstructByFields error, type mismatch : {} != {}", GetTypeInfo(),
                         fields[0]->GetTypeInfo());
  }
  enum_item_ = static_cast<EnumVariable*>(fields[0].get())->enum_item_;
}

void EnumVariable::DefaultConstruct() {
  assert(def_->GetEnumItems().empty() == false);
  enum_item_ = def_->GetEnumItems()[0];
}

std::shared_ptr<Variable> EnumVariable::Clone() {
  auto tmp = std::make_shared<EnumVariable>(def_, Variable::ValueCategory::RValue, "");
  tmp->enum_item_ = enum_item_;
  return tmp;
}

bool EnumVariable::Compare(const std::shared_ptr<Variable>& other) {
  return enum_item_ == static_cast<EnumVariable*>(other.get())->enum_item_;
}

void EnumVariable::Assign(const std::shared_ptr<Variable>& other) {
  enum_item_ = static_cast<EnumVariable*>(other.get())->enum_item_;
}

nlohmann::json EnumVariable::Print() {
  nlohmann::json j;
  j["enum"] = def_->GetEnumName();
  j["enum_item"] = enum_item_;
  return j;
}

void PointerVariable::ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) {
  if (fields.size() != 1) {
    throw WamonException("PointerVariable's ConstructByFields method error : fields.size() == {}", fields.size());
  }
  if (GetTypeInfo() != fields[0]->GetTypeInfo()) {
    throw WamonException("PointerVariable's ConstructByFields method error, type dismatch : {} != {}",
                         fields[0]->GetTypeInfo(), GetTypeInfo());
  }
  obj_ = AsPointerVariable(fields[0])->obj_;
}

void PointerVariable::DefaultConstruct() { obj_.reset(); }

std::shared_ptr<Variable> PointerVariable::Clone() {
  auto ret = std::make_shared<PointerVariable>(GetHoldType(), ValueCategory::RValue, "");
  ret->obj_ = obj_;
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
    throw WamonException("List pop back error, empty list");
  }
  elements_.pop_back();
}

std::string ListVariable::get_string_only_for_byte_list() {
  if (!IsByteType(element_type_)) {
    throw WamonException("ListVariable.get_string_only_for_byte_list can only be called by List(byte) type");
  }
  std::string ret;
  for (auto& each : elements_) {
    ret.push_back(char(AsByteVariable(each)->GetValue()));
  }
  return ret;
}

void ListVariable::ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) {
  for (auto& each : fields) {
    if (each->GetTypeInfo() != element_type_->GetTypeInfo()) {
      throw WamonException("ListVariable::ConstructByFields error, type dismatch : {} != {}", each->GetTypeInfo(),
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

void ListVariable::DefaultConstruct() { elements_.clear(); }

std::shared_ptr<Variable> ListVariable::Clone() {
  std::vector<std::shared_ptr<Variable>> elements;
  for (auto& each : elements_) {
    if (each->IsRValue()) {
      elements.push_back(std::move(each));
    } else {
      elements.push_back(each->Clone());
    }
  }
  auto ret = std::make_shared<ListVariable>(element_type_->Clone(), ValueCategory::RValue, ip_, "");
  ret->elements_ = std::move(elements);
  return ret;
}

void FunctionVariable::ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) {
  if (fields.size() != 1) {
    throw WamonException("FunctionVariable's ConstructByFields method error : fields.size() == {}", fields.size());
  }
  if (IsStructOrEnumType(fields[0]->GetType())) {
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
    throw WamonException("FunctionVariable's ConstructByFields method error, type dismatch : {} != {}",
                         fields[0]->GetTypeInfo(), GetTypeInfo());
  }
  auto other = AsFunctionVariable(fields[0]);
  // 用其他的callable对象构造
  if (other->obj_ != nullptr) {
    if (other->IsRValue()) {
      // move other->obj_ to myself
      obj_ = other->obj_;
      other->obj_.reset();
    } else {
      obj_ = other->obj_->Clone();
    }
    obj_->ChangeTo(vc_);
  }
  if (other->IsRValue()) {
    capture_variables_ = std::move(other->capture_variables_);
  } else {
    capture_variables_.clear();
    for (auto& each : other->capture_variables_) {
      if (each.is_ref == true) {
        assert(!each.v->IsRValue());
        capture_variables_.push_back(each);
      } else {
        auto tmp = each.v->Clone();
        tmp->SetName(each.v->GetName());
        tmp->ChangeTo(vc_);
        capture_variables_.push_back({each.is_ref, tmp});
      }
    }
  }
  func_name_ = other->GetFuncName();
}

void FunctionVariable::DefaultConstruct() {
  obj_ = nullptr;
  func_name_.clear();
  capture_variables_.clear();
}

std::shared_ptr<Variable> FunctionVariable::Clone() {
  std::vector<std::unique_ptr<Type>> param_type;
  auto type = GetType();
  auto fun_type = dynamic_cast<FuncType*>(type.get());
  for (auto&& each : fun_type->GetParamType()) {
    param_type.push_back(each->Clone());
  }
  auto obj = std::make_shared<FunctionVariable>(std::move(param_type), fun_type->GetReturnType()->Clone(),
                                                ValueCategory::RValue, "");
  obj->SetFuncName(func_name_);
  if (obj_) {
    if (IsRValue()) {
      obj->SetObj(std::move(obj_));
    } else {
      obj->SetObj(obj_->Clone());
    }
  }
  if (IsRValue()) {
    obj->capture_variables_ = std::move(capture_variables_);
  } else {
    for (auto& each : capture_variables_) {
      if (each.is_ref == true) {
        obj->capture_variables_.push_back(each);
      } else {
        auto tmp = each.v->Clone();
        tmp->SetName(each.v->GetName());
        obj->capture_variables_.push_back({each.is_ref, tmp});
      }
    }
  }
  return obj;
}

std::shared_ptr<Variable> VariableMove(const std::shared_ptr<Variable>& v) {
  if (v->IsRValue()) {
    return v;
  }
  v->ChangeTo(Variable::ValueCategory::RValue);
  auto tmp = v->Clone();
  tmp->SetName(v->GetName());
  v->DefaultConstruct();
  v->ChangeTo(Variable::ValueCategory::LValue);
  return tmp;
}

std::shared_ptr<Variable> VariableMoveOrCopy(const std::shared_ptr<Variable>& v) {
  if (v->IsRValue()) {
    v->ChangeTo(Variable::ValueCategory::LValue);
    return v;
  }
  auto tmp = v->Clone();
  tmp->SetName(v->GetName());
  tmp->ChangeTo(Variable::ValueCategory::LValue);
  return tmp;
}

}  // namespace wamon
