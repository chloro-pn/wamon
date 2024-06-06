#include "wamon/package_unit.h"

#include "wamon/lambda_function_set.h"
#include "wamon/move_wrapper.h"

namespace wamon {

PackageUnit PackageUnit::_MergePackageUnits(std::vector<PackageUnit>&& packages) {
  PackageUnit result;  // name == ""; import_package == empty; lambda_count == 0;
  for (auto it = packages.begin(); it != packages.end(); ++it) {
    result.AddPackageImports(it->GetName(), it->GetImportPackage());
    for (auto var_define = it->var_define_.begin(); var_define != it->var_define_.end(); ++var_define) {
      auto var_name = (*var_define)->GetVarName();
      (*var_define)->SetVarName(it->GetName() + "$" + var_name);
      result.AddVarDef(std::move(*var_define));
    }

    for (auto func_define = it->funcs_.begin(); func_define != it->funcs_.end(); ++func_define) {
      auto func_name = func_define->first;
      if (!OperatorDef::IsOperatorOverrideName(func_name) && !LambdaExpr::IsLambdaName(func_name)) {
        // 运算符重载不能添加前缀，因为相同类型的重载即便在不同包，也不应该重复定义。
        // lambda函数不添加前缀，因为构造lambda名字的时候已经添加了包名作为后缀
        // todo：支持一个包可以由多个PackageUnit合并得到的情况下，lambda的名字可能会冲突
        func_define->second->SetFunctionName(it->GetName() + "$" + func_name);
      }
      result.AddFuncDef(std::move(func_define->second));
    }

    for (auto struct_define = it->structs_.begin(); struct_define != it->structs_.end(); ++struct_define) {
      auto struct_name = struct_define->first;
      struct_define->second->SetStructName(it->GetName() + "$" + struct_name);
      result.AddStructDef(std::move(struct_define->second));
    }
  }
  // 不需要更新 lambda_count_，因为merge之后的PackageUnit不会再进行解析了。
  return result;
}

void PackageUnit::RegisterCppFunctions(const std::string& name, std::unique_ptr<Type> func_type,
                                       BuiltinFunctions::HandleType ht) {
  if (IsFuncType(func_type) == false) {
    throw WamonException("RegisterCppFunctions error, {} have non-function type : {}", name, func_type->GetTypeInfo());
  }
  auto ftype = func_type->Clone();
  MoveWrapper<decltype(ftype)> mw(std::move(ftype));
  auto check_f = [name = name,
                  mw = mw](const std::vector<std::unique_ptr<Type>>& params_type) mutable -> std::unique_ptr<Type> {
    auto func_type = std::move(mw).Get();
    auto par_type = GetParamType(func_type);
    size_t param_size = par_type.size();
    if (param_size != params_type.size()) {
      throw WamonException("RegisterCppFunctions error, func {} params type count {} != {}", name, params_type.size(),
                           param_size);
    }
    for (size_t i = 0; i < param_size; ++i) {
      if (!IsSameType(par_type[i], params_type[i])) {
        throw WamonException("RegisterCppFunctions error, func {} {}th param type mismatch : {} != {}", name, i,
                             par_type[i]->GetTypeInfo(), params_type[i]->GetTypeInfo());
      }
    }
    return GetReturnType(func_type);
  };
  RegisterCppFunctions(name, std::move(check_f), std::move(ht));
  GetBuiltinFunctions().SetTypeForFunction(name, std::move(func_type));
}

}  // namespace wamon
