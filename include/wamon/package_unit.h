#pragma once

#include <algorithm>
#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "wamon/ast.h"
#include "wamon/builtin_functions.h"
#include "wamon/enum_def.h"
#include "wamon/exception.h"
#include "wamon/function_def.h"
#include "wamon/method_def.h"
#include "wamon/operator_def.h"
#include "wamon/struct_def.h"
#include "wamon/type.h"

namespace wamon {

class PackageUnit {
 public:
  static PackageUnit _MergePackageUnits(std::vector<PackageUnit>&& packages);

  std::string CreateUniqueLambdaName() {
    auto ret = "__lambda_" + std::to_string(lambda_count_) + GetName();
    lambda_count_ += 1;
    return ret;
  }

  PackageUnit() = default;

  PackageUnit(PackageUnit&&) = default;
  PackageUnit& operator=(PackageUnit&&) = default;

  void SetName(const std::string& name) { package_name_ = name; }

  void SetImportPackage(const std::vector<std::string>& import_packages) {
    import_packages_ = import_packages;
    package_imports_[package_name_] = import_packages;
  }

  const std::string& GetName() const { return package_name_; }

  const std::vector<std::string>& GetImportPackage() const { return import_packages_; }

  void AddVarDef(std::unique_ptr<VariableDefineStmt>&& vd) {
    if (std::find_if(var_define_.begin(), var_define_.end(), [&vd](const auto& v) -> bool {
          return vd->GetVarName() == v->GetVarName();
        }) != var_define_.end()) {
      throw WamonException("duplicate global value {}", vd->GetVarName());
    }
    var_define_.push_back(std::move(vd));
  }

  void AddFuncDef(std::unique_ptr<FunctionDef>&& func_def) {
    auto name = func_def->GetFunctionName();
    if (funcs_.find(name) != funcs_.end()) {
      throw WamonException("duplicate func {}", name);
    }
    funcs_[name] = std::move(func_def);
  }

  void AddStructDef(std::unique_ptr<StructDef>&& struct_def) {
    auto name = struct_def->GetStructName();
    if (structs_.find(name) != structs_.end()) {
      throw WamonException("duplicate struct {}", name);
    }
    structs_[name] = std::move(struct_def);
  }

  void AddEnumDef(std::unique_ptr<EnumDef>&& enum_def) {
    auto name = enum_def->GetEnumName();
    if (enum_def->GetEnumItems().empty()) {
      throw WamonException("AddEnumDef error, empty enum {}", name);
    }
    if (enums_.find(name) != enums_.end()) {
      throw WamonException("duplicate enum {}", name);
    }
    enums_[name] = std::move(enum_def);
  }

  void AddMethod(const std::string& type_name, std::unique_ptr<methods_def>&& methods) {
    assert(type_name.empty() == false);
    if (structs_.find(type_name) == structs_.end()) {
      throw WamonException("add method error, invalid type : {}", type_name);
    }
    structs_[type_name]->AddMethods(std::move(methods));
  }

  void AddLambdaFunction(const std::string& lambda_name, std::unique_ptr<FunctionDef>&& lambda) {
    assert(LambdaExpr::IsLambdaName(lambda_name));
    if (funcs_.find(lambda_name) != funcs_.end()) {
      throw WamonException("PackageUnit.AddLambdaFunction error, duplicate function name {}", lambda_name);
    }
    funcs_[lambda_name] = std::move(lambda);
  }

  const StructDef* FindStruct(const std::string& struct_name) const {
    auto it = structs_.find(struct_name);
    if (it == structs_.end()) {
      return nullptr;
    }
    return it->second.get();
  }

  const EnumDef* FindEnum(const std::string& enum_name) const {
    auto it = enums_.find(enum_name);
    if (it == enums_.end()) {
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
      throw WamonException("FindTypeMethod error, type {} not exist", type_name, method_name);
    }
    auto ret = it->second->GetMethod(method_name);
    if (ret == nullptr) {
      throw WamonException("FindTypeMethod error, type {}'s method {} not exist", type_name, method_name);
    }
    return ret;
  }

  std::unique_ptr<Type> GetDataMemberType(const std::string& type_name, const std::string& field_name) const {
    auto it = structs_.find(type_name);
    if (it == structs_.end()) {
      throw WamonException("get data member'type error, type {} not exist", type_name);
    }
    return it->second->GetDataMemberType(field_name);
  }

  const std::vector<std::unique_ptr<VariableDefineStmt>>& GetGlobalVariDefStmt() const { return var_define_; }

  const std::unordered_map<std::string, std::unique_ptr<FunctionDef>>& GetFunctions() const { return funcs_; }

  const std::unordered_map<std::string, std::unique_ptr<StructDef>>& GetStructs() const { return structs_; }

  const BuiltinFunctions& GetBuiltinFunctions() const { return builtin_functions_; }

  BuiltinFunctions& GetBuiltinFunctions() { return builtin_functions_; }

  void AddPackageImports(const std::string& package, const std::vector<std::string>& imports) {
    if (package_imports_.count(package) > 0) {
      auto& tmp = package_imports_[package];
      for (auto& each : imports) {
        tmp.push_back(each);
      }
      return;
    }
    package_imports_[package] = imports;
  }

  const std::vector<std::string>& GetImportsFromPackageName(const std::string& package) const {
    auto it = package_imports_.find(package);
    if (it == package_imports_.end()) {
      throw WamonException("PackageUnit.GetImportsFromPackageName error ,package {} not exist", package);
    }
    return it->second;
  }

  void RegisterCppFunctions(const std::string& name, BuiltinFunctions::CheckType ct, BuiltinFunctions::HandleType ht) {
    GetBuiltinFunctions().Register(name, std::move(ct), std::move(ht));
  }

  void RegisterCppFunctions(const std::string& name, std::unique_ptr<Type> func_type, BuiltinFunctions::HandleType ht);

  const std::vector<std::string>& GetCurrentParsingImports() const {
    return GetImportsFromPackageName(current_parsing_package_);
  }

  const std::string& GetCurrentParsingPackage() const { return current_parsing_package_; }

  void SetCurrentParsingPackage(const std::string& cpp) { current_parsing_package_ = cpp; }

 private:
  std::string package_name_;
  std::vector<std::string> import_packages_;
  // 包作用域的变量定义语句
  std::vector<std::unique_ptr<VariableDefineStmt>> var_define_;
  std::unordered_map<std::string, std::unique_ptr<FunctionDef>> funcs_;
  std::unordered_map<std::string, std::unique_ptr<StructDef>> structs_;
  std::unordered_map<std::string, std::unique_ptr<EnumDef>> enums_;
  BuiltinFunctions builtin_functions_;
  size_t lambda_count_{0};
  // 只有通过Merge生成的PackageUnit才使用这项
  std::unordered_map<std::string, std::vector<std::string>> package_imports_;

  // 以下成员只有在parse对应的包的时候，以及执行ExecExpression的时候才会被设置
  std::string current_parsing_package_;
};

template <typename... T>
PackageUnit MergePackageUnits(T&&... package) {
  std::vector<PackageUnit> tmp;
  detail::elements_append_to(tmp, std::forward<T>(package)...);
  return PackageUnit::_MergePackageUnits(std::move(tmp));
}

}  // namespace wamon
