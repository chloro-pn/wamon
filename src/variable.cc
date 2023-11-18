#include "wamon/variable.h"
#include "wamon/interpreter.h"
#include "wamon/struct_def.h"

namespace wamon {

std::unique_ptr<Variable> VariableFactory(std::unique_ptr<Type> type, const std::string& name, Interpreter& interpreter) {
  if (IsBuiltInType(type)) {
    std::string typeinfo = type->GetTypeInfo();
    if (typeinfo == "string") {
      return std::make_unique<StringVariable>("", name);
    }
    if (typeinfo == "void") {
      return std::make_unique<VoidVariable>();
    }
    if (typeinfo == "bool") {
      return std::make_unique<BoolVariable>(true, name);
    }
    if (typeinfo == "int") {
      return std::make_unique<IntVariable>(0, name);
    }
    if (typeinfo == "double") {
      return std::make_unique<DoubleVariable>(0.0, name);
    }
    if (typeinfo == "byte") {
      return std::make_unique<ByteVariable>(0, name);
    }
  }
  if (type->IsBasicType() == true) {
    auto struct_def = interpreter.GetPackageUnit().FindStruct(type->GetTypeInfo());
    assert(struct_def != nullptr);
    return std::make_unique<StructVariable>(struct_def, interpreter, name);
  }
  if (IsPtrType(type)) {
    return std::make_unique<PointerVariable>(GetHoldType(type), name);
  }
  if (IsListType(type)) {
    return std::make_unique<ListVariable>(GetElementType(type), name);
  }
  if (IsFuncType(type)) {
    return std::make_unique<FunctionVariable>(GetParamType(type), GetReturnType(type), name);
  }
  throw WamonExecption("VariableFactory error, not implement now.");
}

StructVariable::StructVariable(const StructDef* sd, Interpreter& i, const std::string& name) : 
    Variable(std::make_unique<BasicType>(sd->GetStructName()), name),
    def_(sd),
    interpreter_(i) {

  }

std::shared_ptr<Variable> StructVariable::GetDataMemberByName(const std::string& name) {
  for(auto& each : data_members_) {
    if (each.name == name) {
      return each.data;
    }
  }
  return nullptr;
}

void StructVariable::ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) {
  auto& members = def_->GetDataMembers();
  if (fields.size() != members.size()) {
    throw WamonExecption("StructVariable's ConstructByFields method error : fields.size() == {}", fields.size());
  }
  for(size_t i = 0; i < members.size(); ++i) {
    if (fields[i]->GetTypeInfo() != members[i].second->GetTypeInfo()) {
      throw WamonExecption("StructVariable's ConstructByFields method error : {}th type dismatch : {} != {}", i, fields[i]->GetTypeInfo(), members[i].second->GetTypeInfo());
    }
    data_members_.push_back({members[i].first, fields[i]->Clone()});
  }
}

void StructVariable::DefaultConstruct() {
  auto& members = def_->GetDataMembers();
  for(auto& each : members) {
    data_members_.push_back({each.first, VariableFactory(each.second->Clone(), each.first, interpreter_)});
  }
}

std::unique_ptr<Variable> StructVariable::Clone() {
  std::vector<std::shared_ptr<Variable>> variables;
  for(auto& each : data_members_) {
    variables.push_back(each.data->Clone());
  }
  auto ret = std::make_unique<StructVariable>(def_, interpreter_, GetName());
  ret->ConstructByFields(variables);
  return ret;
}

void PointerVariable::ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) {
  if (fields.size() != 1) {
    throw WamonExecption("PointerVariable's ConstructByFields method error : fields.size() == {}", fields.size());
  }
  if (GetTypeInfo() != fields[0]->GetTypeInfo()) {
    throw WamonExecption("PointerVariable's ConstructByFields method error, type dismatch : {} != {}", fields[0]->GetTypeInfo(), GetTypeInfo());
  }
  obj_ = AsPointerVariable(fields[0])->GetHoldVariable();
}

void PointerVariable::DefaultConstruct() {
  obj_.reset();
}

std::unique_ptr<Variable> PointerVariable::Clone() {
  auto ret = std::make_unique<PointerVariable>(obj_.lock()->GetType(), "");
  ret->SetHoldVariable(obj_.lock());
  return ret;
}

void ListVariable::PushBack(std::shared_ptr<Variable> element) {
  elements_.push_back(std::move(element));
}

void ListVariable::PopBack() {
  if (elements_.empty()) {
    throw WamonExecption("List pop back error, empty list");
  }
  elements_.pop_back();
}

void ListVariable::ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) {
  for(auto& each : fields) {
    if (each->GetTypeInfo() != element_type_->GetTypeInfo()) {
      throw WamonExecption("ListVariable::ConstructByFields error, type dismatch : {} != {}", each->GetTypeInfo(), element_type_->GetTypeInfo());
    }
  }
  elements_ = std::move(fields);
}

void ListVariable::DefaultConstruct() {
  
}

std::unique_ptr<Variable> ListVariable::Clone() {
  std::vector<std::shared_ptr<Variable>> elements;
  for(auto& each : elements_) {
    elements.push_back(each->Clone());
  }
  auto ret = std::make_unique<ListVariable>(element_type_->Clone(), "");
  ret->elements_ = std::move(elements);
  return ret;
}

void FunctionVariable::ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) {
  if (fields.size() != 1) {
    throw WamonExecption("FunctionVariable's ConstructByFields method error : fields.size() == {}", fields.size());
  }
  if (IsBasicType(fields[0]->GetType()) && !IsBuiltInType(fields[0]->GetType())) {
    // structtype
    obj_ = fields[0]->Clone();
    return;
  }
  if (fields[0]->GetTypeInfo() != GetTypeInfo()) {
    throw WamonExecption("FunctionVariable's ConstructByFields method error, type dismatch : {} != {}", fields[0]->GetTypeInfo(), GetTypeInfo());
  }
  func_name_ = AsStringVariable(fields[0])->GetValue();
}

void FunctionVariable::DefaultConstruct() {
  
}

std::unique_ptr<Variable> FunctionVariable::Clone() {
  std::vector<std::unique_ptr<Type>> param_type;
  auto type = GetType();
  auto fun_type = dynamic_cast<FuncType*>(type.get());
  for(auto&& each : fun_type->GetParamType()) {
    param_type.push_back(each->Clone());
  }
  auto obj = std::make_unique<FunctionVariable>(std::move(param_type), fun_type->GetReturnType()->Clone(), "");
  obj->SetFuncName(func_name_);
  obj->SetObj(obj_ == nullptr ? nullptr : obj_->Clone());
  return obj;
}

}