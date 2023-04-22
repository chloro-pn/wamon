#include <unordered_map>
#include <string>
#include <memory>
#include <exception>
#include <stdexcept>

#include "wamon/variable.h"
#include "wamon/exception.h"

namespace wamon {
/*
 * 全局上下文、函数上下文、块上下文
 */
class Context {
 public:
  enum class Type { GLOBAL, FUNC, BLOCK };
  
  explicit Context(Type type) : type_(type) {}

  void RegisterVariable(std::unique_ptr<Variable>&& v) {
    if (v == nullptr || v->GetName() == "" || vars_.find(v->GetName()) != vars_.end()) {
      throw WamonExecption("register variable error");
    }
    std::string name = v->GetName();
    vars_[name] = std::move(v);
  }

  // 在当前上下文查找变量，注意调用者不获得该变量的所有权
  Variable* GetVariableByName(const std::string& name) {
    auto it = vars_.find(name);
    if (it == vars_.end()) {
      return nullptr;
    }
    return it->second.get();
  }

 private:
  Type type_;
  std::unordered_map<std::string, std::unique_ptr<Variable>> vars_;
};

}