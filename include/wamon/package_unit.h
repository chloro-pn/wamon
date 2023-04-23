#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cassert>

#include "wamon/ast.h"
#include "wamon/function_def.h"
#include "wamon/struct_def.h"
#include "wamon/method_def.h"
#include "wamon/exception.h"

namespace wamon {

class PackageUnit {
 public:
  PackageUnit() = default;

  PackageUnit(PackageUnit&&) = default;
  PackageUnit& operator=(PackageUnit&&) = default;

  void SetName(const std::string& name) { package_name_ = name; }
  void SetImportPackage(const std::vector<std::string>& import_packages) { import_packages_ = import_packages; }
  void AddVarDef(std::unique_ptr<VariableDefineStmt>&& vd) {
    var_define_.push_back(std::move(vd));
  }

  void AddFuncDef(std::unique_ptr<FunctionDef>&& func_def) {
    auto name = func_def->GetFunctionName();
    funcs_[name] = std::move(func_def);
  }

  void AddStructDef(std::unique_ptr<StructDef>&& struct_def) {
    auto name = struct_def->GetStructName();
    structs_[name] = std::move(struct_def);
  }

  void AddMethod(const std::string& type_name, std::unique_ptr<methods_def>&& methods) {
    assert(type_name.empty() == false);
    if (structs_.find(type_name) == structs_.end()) {
      throw WamonExecption("add method error, invalid type : {}", type_name);
    }
    structs_[type_name]->AddMethods(std::move(methods));
  }
  
 private:
  std::string package_name_;
  std::vector<std::string> import_packages_;
  // 包作用域的变量定义语句
  std::vector<std::unique_ptr<VariableDefineStmt>> var_define_;
  std::unordered_map<std::string, std::unique_ptr<FunctionDef>> funcs_;
  std::unordered_map<std::string, std::unique_ptr<StructDef>> structs_;
};

}