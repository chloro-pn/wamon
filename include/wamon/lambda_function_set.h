#pragma once

#include <string>
#include <unordered_map>

#include "wamon/exception.h"
#include "wamon/function_def.h"

namespace wamon {

class LambdaFunctionSet {
 public:
  static LambdaFunctionSet& Instance() {
    static LambdaFunctionSet obj;
    return obj;
  }

  void RegisterLambdaFunction(const std::string& name, std::unique_ptr<FunctionDef>&& lambda) {
    if (funcs_.find(name) != funcs_.end()) {
      throw WamonExecption("LambdaFunctionSet.RegisterLambdaFunction error, duplicate name {}", name);
    }
    funcs_.insert({name, std::move(lambda)});
  }

  std::unordered_map<std::string, std::unique_ptr<FunctionDef>> GetAllLambdas() { return std::move(funcs_); }

 private:
  LambdaFunctionSet() = default;

  std::unordered_map<std::string, std::unique_ptr<FunctionDef>> funcs_;
};

}  // namespace wamon