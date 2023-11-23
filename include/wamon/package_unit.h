#pragma once

#include <algorithm>
#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "wamon/ast.h"
#include "wamon/exception.h"
#include "wamon/function_def.h"
#include "wamon/method_def.h"
#include "wamon/operator_def.h"
#include "wamon/struct_def.h"

namespace wamon {

class PackageUnit {
 public:
  PackageUnit() = default;

  PackageUnit(PackageUnit&&) = default;
  PackageUnit& operator=(PackageUnit&&) = default;

  void SetName(const std::string& name) { package_name_ = name; }
  void SetImportPackage(const std::vector<std::string>& import_packages) { import_packages_ = import_packages; }
  void AddVarDef(std::unique_ptr<VariableDefineStmt>&& vd) {
    if (std::find_if(var_define_.begin(), var_define_.end(), [&vd](const auto& v) -> bool {
          return vd->GetVarName() == v->GetVarName();
        }) != var_define_.end()) {
      throw WamonExecption("duplicate global value {}", vd->GetVarName());
    }
    var_define_.push_back(std::move(vd));
  }

  void AddFuncDef(std::unique_ptr<FunctionDef>&& func_def) {
    auto name = func_def->GetFunctionName();
    if (funcs_.find(name) != funcs_.end()) {
      throw WamonExecption("duplicate func {}", func_def->GetFunctionName());
    }
    funcs_[name] = std::move(func_def);
  }

  void AddStructDef(std::unique_ptr<StructDef>&& struct_def) {
    auto name = struct_def->GetStructName();
    if (structs_.find(name) != structs_.end()) {
      throw WamonExecption("duplicate struct {}", struct_def->GetStructName());
    }
    structs_[name] = std::move(struct_def);
  }

  void AddMethod(const std::string& type_name, std::unique_ptr<methods_def>&& methods) {
    assert(type_name.empty() == false);
    if (structs_.find(type_name) == structs_.end()) {
      throw WamonExecption("add method error, invalid type : {}", type_name);
    }
    structs_[type_name]->AddMethods(std::move(methods));
  }

  const StructDef* FindStruct(const std::string& struct_name) const {
    auto it = structs_.find(struct_name);
    if (it == structs_.end()) {
      return nullptr;
    }
    return it->second.get();
  }

  const FunctionDef* FindFunction(const std::string& function_name) const {
    auto it = funcs_.find(function_name);
    if (it == funcs_.end()) {
      return nullptr;
    }
    return it->second.get();
  }

  const MethodDef* FindTypeMethod(const std::string& type_name, const std::string& method_name) const {
    auto it = structs_.find(type_name);
    if (it == structs_.end()) {
      throw WamonExecption("find type method error, type {} not exist", type_name, method_name);
    }
    return it->second->GetMethod(method_name);
  }

  std::unique_ptr<Type> GetDataMemberType(const std::string& type_name, const std::string& field_name) const {
    auto it = structs_.find(type_name);
    if (it == structs_.end()) {
      throw WamonExecption("get data member'type error, type {} not exist", type_name);
    }
    return it->second->GetDataMemberType(field_name);
  }

  const std::vector<std::unique_ptr<VariableDefineStmt>>& GetGlobalVariDefStmt() const { return var_define_; }

  const std::unordered_map<std::string, std::unique_ptr<FunctionDef>>& GetFunctions() const { return funcs_; }

  const std::unordered_map<std::string, std::unique_ptr<StructDef>>& GetStructs() const { return structs_; }

 private:
  std::string package_name_;
  std::vector<std::string> import_packages_;
  // 包作用域的变量定义语句
  std::vector<std::unique_ptr<VariableDefineStmt>> var_define_;
  std::unordered_map<std::string, std::unique_ptr<FunctionDef>> funcs_;
  std::unordered_map<std::string, std::unique_ptr<StructDef>> structs_;
};

}  // namespace wamon
