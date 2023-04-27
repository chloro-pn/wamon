#include <unordered_map>
#include <string>
#include <memory>
#include <exception>
#include <stdexcept>
#include <optional>
#include <cassert>

#include "wamon/exception.h"
#include "wamon/type.h"

namespace wamon {
/*
 * 全局上下文、函数上下文、块上下文
 */
class Context {
 public:
  enum class ContextType { GLOBAL, FUNC, BLOCK, METHOD, WHILE_BLOCK, FOR_BLOCK };

  explicit Context(ContextType type) : type_(type) {
    // method and func context使用其他构造函数
    assert(type != ContextType::METHOD);
    assert(type != ContextType::FUNC);
  }

  explicit Context(const std::string& type_name, const std::string& method_name) : 
      type_(ContextType::METHOD), 
      method_context_type_name_(type_name),
      method_context_method_name_(method_name) {}

  explicit Context(const std::string& func_name) :
      type_(ContextType::FUNC),
      func_context_func_name_(func_name) {}

  ContextType GetType() const {
    return type_;
  }

  const std::string& AssertMethodContextAndGetTypeName() {
    if (type_ != ContextType::METHOD) {
      throw WamonExecption("get context's type in non-method context");
    }
    return method_context_type_name_;
  }

  const std::string& AssertMethodContextAndGetMethodName() {
    if (type_ != ContextType::METHOD) {
      throw WamonExecption("get context's method_name in non-method context");
    }
    return method_context_method_name_;
  }

  const std::string& AssertFuncContextAndGetFuncName() {
    if (type_ != ContextType::FUNC) {
      throw WamonExecption("assert context's type error, should be func");
    }
    return func_context_func_name_;
  }

  void RegisterVariable(const std::string& name, std::unique_ptr<Type>&& type) {
    if (vars_.find(name) != vars_.end()) {
      throw WamonExecption("register variable error, duplicate name {}", name);
    }
    vars_[name] = std::move(type);
  }

  // 在当前上下文查找变量类型
  std::unique_ptr<Type> GetTypeByName(const std::string& name) const {
    auto it = vars_.find(name);
    if (it == vars_.end()) {
      return nullptr;
    }
    return it->second->Clone();
  }

 private:
  ContextType type_;
  std::string method_context_type_name_;
  std::string method_context_method_name_;
  std::string func_context_func_name_;
  // 每个上下文都存储着变量集合以及其类型，也就是说同一个上下文内的变量不能是重名的。
  std::unordered_map<std::string, std::unique_ptr<Type>> vars_;
};

}