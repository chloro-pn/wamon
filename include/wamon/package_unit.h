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
#include "wamon/prepared_package_name.h"
#include "wamon/struct_def.h"
#include "wamon/type.h"

namespace wamon {

struct UsingItem {
  std::string package_name;
  enum class using_type { TOTAL, ITEM };
  using_type type;
  std::set<std::string> using_set;
};

class PackageUnit {
 private:
  static PackageUnit _MergePackageUnits(std::vector<PackageUnit>&& packages);

  static PackageUnit GetWamonPackage();

 public:
  std::string CreateUniqueLambdaName() {
    MergedFlagCheck("CreateUniqueLambdaName");
    auto ret = "__lambda_" + std::to_string(lambda_count_) + GetName();
    lambda_count_ += 1;
    return ret;
  }

  PackageUnit() = default;

  PackageUnit(PackageUnit&&) = default;
  PackageUnit& operator=(PackageUnit&&) = default;

  void SetName(const std::string& name) {
    MergedFlagCheck("SetName");
    if (IsPreparedPakageName(name)) {
      throw WamonException("PackageUnit.SetName invalid , {}", name);
    }
    package_name_ = name;
  }

  void SetImportPackage(const std::vector<std::string>& import_packages) {
    MergedFlagCheck("SetImportPackage");
    import_packages_ = import_packages;
    package_imports_[package_name_] = import_packages;
  }

  void SetUsingPackage(const std::unordered_map<std::string, UsingItem>& using_packages) {
    MergedFlagCheck("SetUsingPackage");
    using_packages_ = using_packages;
  }

  const std::string& GetName() const { return package_name_; }

  const std::vector<std::string>& GetImportPackage() const { return import_packages_; }

  bool InImportPackage(const std::string& package_name) const {
    auto it = std::find(import_packages_.begin(), import_packages_.end(), package_name);
    return it != import_packages_.end();
  }

  const std::unordered_map<std::string, UsingItem>& GetUsingPackage() const { return using_packages_; }

  void AddVarDef(const std::shared_ptr<VariableDefineStmt>& vd) {
    MergedFlagCheck("AddVarDef");
    if (std::find_if(var_define_.begin(), var_define_.end(), [&vd](const auto& v) -> bool {
          return vd->GetVarName() == v->GetVarName();
        }) != var_define_.end()) {
      throw WamonException("duplicate global value {}", vd->GetVarName());
    }
    var_define_.push_back(vd);
  }

  void AddFuncDef(const std::shared_ptr<FunctionDef>& func_def) {
    MergedFlagCheck("AddFuncDef");
    auto name = func_def->GetFunctionName();
    if (funcs_.find(name) != funcs_.end()) {
      throw WamonException("duplicate func {}", name);
    }
    funcs_[name] = func_def;
  }

  void AddStructDef(const std::shared_ptr<StructDef>& struct_def) {
    MergedFlagCheck("AddStructDef");
    auto name = struct_def->GetStructName();
    if (structs_.find(name) != structs_.end()) {
      throw WamonException("duplicate struct {}", name);
    }
    structs_[name] = struct_def;
  }

  void AddEnumDef(const std::shared_ptr<EnumDef>& enum_def) {
    MergedFlagCheck("AddEnumDef");
    auto name = enum_def->GetEnumName();
    if (enum_def->GetEnumItems().empty()) {
      throw WamonException("AddEnumDef error, empty enum {}", name);
    }
    if (enums_.find(name) != enums_.end()) {
      throw WamonException("duplicate enum {}", name);
    }
    enums_[name] = enum_def;
  }

  void AddMethod(const std::string& type_name, std::unique_ptr<methods_def>&& methods) {
    MergedFlagCheck("AddMethod");
    assert(type_name.empty() == false);
    if (structs_.find(type_name) == structs_.end()) {
      throw WamonException("add method error, invalid type : {}", type_name);
    }
    structs_[type_name]->AddMethods(std::move(methods));
  }

  void AddLambdaFunction(const std::string& lambda_name, std::unique_ptr<FunctionDef>&& lambda) {
    MergedFlagCheck("AddLambdaFunction");
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

  const std::vector<std::shared_ptr<VariableDefineStmt>>& GetGlobalVariDefStmt() const { return var_define_; }

  const std::unordered_map<std::string, std::shared_ptr<FunctionDef>>& GetFunctions() const { return funcs_; }

  const std::unordered_map<std::string, std::shared_ptr<StructDef>>& GetStructs() const { return structs_; }

  const BuiltinFunctions& GetBuiltinFunctions() const { return builtin_functions_; }

  BuiltinFunctions& GetBuiltinFunctions() { return builtin_functions_; }

  void AddPackageImports(const std::string& package, const std::vector<std::string>& imports) {
    MergedFlagCheck("AddPackageImports");
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
    MergedFlagCheck("RegisterCppFunctions");
    GetBuiltinFunctions().Register(GetName() + "$" + name, std::move(ct), std::move(ht));
  }

  void RegisterCppFunctions(const std::string& name, std::unique_ptr<Type> func_type, BuiltinFunctions::HandleType ht);

  const std::vector<std::string>& GetCurrentParsingImports() const {
    return GetImportsFromPackageName(current_parsing_package_);
  }

  const std::string& GetCurrentParsingPackage() const { return current_parsing_package_; }

  void SetCurrentParsingPackage(const std::string& cpp) { current_parsing_package_ = cpp; }

 private:
  // PackageUnit有两种类型，只有Merge得到的PackageUnit才能够进行类型分析以及交付给解释器执行。
  bool merged = false;

  void MergedFlagCheck(const std::string& info) {
    if (merged == true) {
      throw WamonException("MergedFlagCheck error, from {}", info);
    }
  }

  template <typename... T>
  friend PackageUnit MergePackageUnits(T&&... package);

  std::string package_name_;
  std::vector<std::string> import_packages_;
  std::unordered_map<std::string, UsingItem> using_packages_;
  // 包作用域的变量定义语句
  std::vector<std::shared_ptr<VariableDefineStmt>> var_define_;
  std::unordered_map<std::string, std::shared_ptr<FunctionDef>> funcs_;
  std::unordered_map<std::string, std::shared_ptr<StructDef>> structs_;
  std::unordered_map<std::string, std::shared_ptr<EnumDef>> enums_;
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
