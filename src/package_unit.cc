#include "wamon/package_unit.h"

#include "wamon/lambda_function_set.h"

namespace wamon {

PackageUnit PackageUnit::_MergePackageUnits(std::vector<PackageUnit>&& packages) {
  PackageUnit result;  // name == ""; import_package == empty;
  for (auto it = packages.begin(); it != packages.end(); ++it) {
    auto package_name = it->package_name_;
    for (auto var_define = it->var_define_.begin(); var_define != it->var_define_.end(); ++var_define) {
      auto var_name = (*var_define)->GetVarName();
      (*var_define)->SetVarName(package_name + "$" + var_name);
      result.AddVarDef(std::move(*var_define));
    }

    for (auto func_define = it->funcs_.begin(); func_define != it->funcs_.end(); ++func_define) {
      auto func_name = func_define->first;
      if (!OperatorDef::IsOperatorOverrideName(func_name) && !LambdaExpr::IsLambdaName(func_name)) {
        // 运算符重载不能添加前缀，因为相同类型的重载即便在不同包，也不应该重复定义。
        // lambda函数不添加前缀
        func_define->second->SetFunctionName(package_name + "$" + func_name);
      }
      result.AddFuncDef(std::move(func_define->second));
    }

    for (auto struct_define = it->structs_.begin(); struct_define != it->structs_.end(); ++struct_define) {
      auto struct_name = struct_define->first;
      struct_define->second->SetStructName(package_name + "$" + struct_name);
      result.AddStructDef(std::move(struct_define->second));
    }
  }
  return result;
}

}  // namespace wamon
