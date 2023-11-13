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
  throw WamonExecption("VariableFactory error, not implement now.");
}

StructVariable::StructVariable(const StructDef* sd, Interpreter& i, const std::string& name) : 
    Variable(std::make_unique<BasicType>(sd->GetStructName()), name),
    def_(sd),
    interpreter_(i) {

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

}